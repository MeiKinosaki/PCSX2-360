#include "ee_state.h"
#include "ee_memory.h"
#include <string.h>

#define SNAPSHOT_HEADER_SIZE 16u
#define SNAPSHOT_MAGIC_EE UINT32_C(0x53324545)
#define SNAPSHOT_MAGIC_MACHINE UINT32_C(0x4d325345)

static void put_u32(uint8_t *p, uint32_t v)
{
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24);
}
static uint32_t get_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
static uint32_t checksum(const uint8_t *p, size_t n)
{
    uint32_t h=UINT32_C(2166136261); size_t i;
    for(i=0;i<n;i++){ h^=p[i]; h*=UINT32_C(16777619); }
    return h;
}

#define EE_SNAPSHOT_BYTES PS2_EE_SNAPSHOT_PAYLOAD_SIZE
#define TLB_ENTRY_BYTES 39u
_Static_assert(TLB_ENTRY_BYTES == 4u + 4u + 3u + 2u * (4u + 3u), "TLB snapshot wire size mismatch");
#define TLB_SNAPSHOT_BYTES (1u + 48u * TLB_ENTRY_BYTES)

static void put_u64(uint8_t *p, uint64_t v)
{
    put_u32(p, (uint32_t)v);
    put_u32(p + 4, (uint32_t)(v >> 32));
}
static uint64_t get_u64(const uint8_t *p)
{
    return (uint64_t)get_u32(p) | ((uint64_t)get_u32(p + 4) << 32);
}
static uint8_t *encode_ee(uint8_t *p, const ps2_ee_state_t *s)
{
    unsigned i;
    for (i = 0; i < PS2_EE_GPR_COUNT; ++i, p += 8) put_u64(p, s->gpr[i]);
    put_u64(p, s->hi); p += 8; put_u64(p, s->lo); p += 8;
#define PUT_EE32(field) do { put_u32(p, s->field); p += 4; } while (0)
    PUT_EE32(pc); PUT_EE32(next_pc); PUT_EE32(branch_pc);
    *p++ = s->in_delay_slot;
    PUT_EE32(cause); PUT_EE32(status); PUT_EE32(epc); PUT_EE32(badvaddr);
    PUT_EE32(cp0_context); PUT_EE32(cp0_index); PUT_EE32(cp0_random);
    PUT_EE32(cp0_entry_lo0); PUT_EE32(cp0_entry_lo1); PUT_EE32(cp0_page_mask);
    PUT_EE32(cp0_wired); PUT_EE32(cp0_entry_hi);
#undef PUT_EE32
    put_u64(p, s->cycle_count); p += 8;
    return p;
}
static const uint8_t *decode_ee(ps2_ee_state_t *s, const uint8_t *p)
{
    unsigned i;
    memset(s, 0, sizeof(*s));
    for (i = 0; i < PS2_EE_GPR_COUNT; ++i, p += 8) s->gpr[i] = get_u64(p);
    s->hi = get_u64(p); p += 8; s->lo = get_u64(p); p += 8;
#define GET_EE32(field) do { s->field = get_u32(p); p += 4; } while (0)
    GET_EE32(pc); GET_EE32(next_pc); GET_EE32(branch_pc);
    s->in_delay_slot = *p++;
    GET_EE32(cause); GET_EE32(status); GET_EE32(epc); GET_EE32(badvaddr);
    GET_EE32(cp0_context); GET_EE32(cp0_index); GET_EE32(cp0_random);
    GET_EE32(cp0_entry_lo0); GET_EE32(cp0_entry_lo1); GET_EE32(cp0_page_mask);
    GET_EE32(cp0_wired); GET_EE32(cp0_entry_hi);
#undef GET_EE32
    s->cycle_count = get_u64(p); p += 8;
    return p;
}
static uint8_t *encode_tlb(uint8_t *p, const ps2_ee_tlb_arch_t *t)
{
    unsigned i;
    *p++ = t->current_asid;
    for (i = 0; i < 48u; ++i) {
        const ps2_ee_tlb_pair_entry_t *e = &t->entries[i];
        put_u32(p, e->vpn2); p += 4; put_u32(p, e->page_mask); p += 4;
        *p++ = e->asid; *p++ = e->global; *p++ = e->present;
#define PUT_HALF(h) do { put_u32(p, e->h.physical_page); p += 4; *p++ = e->h.valid; *p++ = e->h.dirty; *p++ = e->h.global; } while (0)
        PUT_HALF(even_page); PUT_HALF(odd_page);
#undef PUT_HALF
    }
    return p;
}
static const uint8_t *decode_tlb(ps2_ee_tlb_arch_t *t, const uint8_t *p)
{
    unsigned i;
    memset(t, 0, sizeof(*t)); t->current_asid = *p++;
    for (i = 0; i < 48u; ++i) {
        ps2_ee_tlb_pair_entry_t *e = &t->entries[i];
        e->vpn2 = get_u32(p); p += 4; e->page_mask = get_u32(p); p += 4;
        e->asid = *p++; e->global = *p++; e->present = *p++;
#define GET_HALF(h) do { e->h.physical_page = get_u32(p); p += 4; e->h.valid = *p++; e->h.dirty = *p++; e->h.global = *p++; } while (0)
        GET_HALF(even_page); GET_HALF(odd_page);
#undef GET_HALF
    }
    return p;
}

/* Reject malformed serialized TLB slots before restoring any machine state. */
static int snapshot_tlb_valid(const ps2_ee_tlb_arch_t *tlb)
{
    unsigned i;
    if (tlb == NULL) return 0;
    for (i = 0; i < 48u; ++i) {
        const ps2_ee_tlb_pair_entry_t *e = &tlb->entries[i];
        uint32_t m, compare_mask;
        if (e->present > 1u || e->global > 1u) return 0;
        if (!e->present) continue;
        if ((e->page_mask & ~UINT32_C(0x01ffe000)) != 0u) return 0;
        m = (e->page_mask & UINT32_C(0x01ffe000)) >> 13;
        switch (m) {
        case 0x000u: case 0x003u: case 0x00fu: case 0x03fu:
        case 0x0ffu: case 0x3ffu: case 0xfffu:
            break;
        default:
            return 0;
        }
        compare_mask = ~(e->page_mask | UINT32_C(0x1fff));
        if ((e->vpn2 & ~compare_mask) != 0u) return 0;
        if (e->even_page.valid > 1u || e->even_page.dirty > 1u ||
            e->even_page.global > 1u || e->odd_page.valid > 1u ||
            e->odd_page.dirty > 1u || e->odd_page.global > 1u)
            return 0;
        if ((e->even_page.physical_page & UINT32_C(0xfff)) != 0u ||
            (e->odd_page.physical_page & UINT32_C(0xfff)) != 0u)
            return 0;
        if (e->global != (uint8_t)(e->even_page.global &&
                                   e->odd_page.global)) return 0;
    }
    return 1;
}

int ps2_ee_state_save(const ps2_ee_state_t *state, uint8_t *out, size_t capacity, size_t *written)
{
    size_t payload, total;
    if(written) *written=0u;
    if(!state || !out || !written) return -1;
    if(state->gpr[0]!=0u || state->in_delay_slot>1u) return -3;
    payload=EE_SNAPSHOT_BYTES; total=SNAPSHOT_HEADER_SIZE+payload;
    if(capacity<total) return -2;
    put_u32(out,SNAPSHOT_MAGIC_EE); put_u32(out+4,PS2_EE_SNAPSHOT_VERSION);
    put_u32(out+8,(uint32_t)payload);
    (void)encode_ee(out+SNAPSHOT_HEADER_SIZE,state);
    put_u32(out+12,checksum(out+SNAPSHOT_HEADER_SIZE,payload));
    *written=total;
    return 0;
}

int ps2_ee_state_load(ps2_ee_state_t *state, const uint8_t *data, size_t size)
{
    ps2_ee_state_t candidate; uint32_t payload;
    if(!state || !data) return -1;
    if(size<SNAPSHOT_HEADER_SIZE || get_u32(data)!=SNAPSHOT_MAGIC_EE) return -2;
    if(get_u32(data+4)!=PS2_EE_SNAPSHOT_VERSION) return -3;
    payload=get_u32(data+8);
    if(payload!=EE_SNAPSHOT_BYTES || size!=SNAPSHOT_HEADER_SIZE+(size_t)payload) return -4;
    if(checksum(data+SNAPSHOT_HEADER_SIZE,payload)!=get_u32(data+12)) return -5;
    (void)decode_ee(&candidate,data+SNAPSHOT_HEADER_SIZE);
    if(candidate.gpr[0]!=0u || candidate.in_delay_slot>1u) return -6;
    *state=candidate;
    return 0;
}

int ps2_ee_machine_snapshot_save(const ps2_ee_state_t *state, const ps2_ee_tlb_arch_t *tlb,
                                 const ps2_ee_memory_t *memory, uint8_t *out,
                                 size_t capacity, size_t *written)
{
    size_t payload,total; uint8_t *p;
    if(written) *written=0u;
    if(!state || !tlb || !memory || !memory->ram || !out || !written) return -1;
    if(!snapshot_tlb_valid(tlb)) return -4;
    if(state->gpr[0]!=0u || state->in_delay_slot>1u) return -5;
    if(memory->ram_size>UINT32_MAX ||
       memory->ram_size>SIZE_MAX-EE_SNAPSHOT_BYTES-TLB_SNAPSHOT_BYTES-4u-SNAPSHOT_HEADER_SIZE) return -2;
    payload=EE_SNAPSHOT_BYTES+TLB_SNAPSHOT_BYTES+4u+memory->ram_size;
    total=SNAPSHOT_HEADER_SIZE+payload;
    if(capacity<total) return -3;
    put_u32(out,SNAPSHOT_MAGIC_MACHINE); put_u32(out+4,PS2_EE_MACHINE_SNAPSHOT_VERSION); put_u32(out+8,(uint32_t)payload);
    p=encode_ee(out+SNAPSHOT_HEADER_SIZE,state);
    p=encode_tlb(p,tlb);
    put_u32(p,(uint32_t)memory->ram_size); p+=4;
    memcpy(p,memory->ram,memory->ram_size);
    put_u32(out+12,checksum(out+SNAPSHOT_HEADER_SIZE,payload));
    *written=total;
    return 0;
}

int ps2_ee_machine_snapshot_load(ps2_ee_state_t *state, ps2_ee_tlb_arch_t *tlb,
                                 ps2_ee_memory_t *memory, const uint8_t *data, size_t size)
{
    ps2_ee_state_t candidate_state; ps2_ee_tlb_arch_t candidate_tlb;
    uint32_t payload,ram_size; const uint8_t *p;
    if(!state || !tlb || !memory || !memory->ram || !data) return -1;
    if(size<SNAPSHOT_HEADER_SIZE || get_u32(data)!=SNAPSHOT_MAGIC_MACHINE) return -2;
    if(get_u32(data+4)!=PS2_EE_MACHINE_SNAPSHOT_VERSION) return -3;
    payload=get_u32(data+8);
    if(size!=SNAPSHOT_HEADER_SIZE+(size_t)payload) return -4;
    if(checksum(data+SNAPSHOT_HEADER_SIZE,payload)!=get_u32(data+12)) return -5;
    if(payload<EE_SNAPSHOT_BYTES+TLB_SNAPSHOT_BYTES+4u) return -6;
    p=data+SNAPSHOT_HEADER_SIZE;
    p=decode_ee(&candidate_state,p);
    p=decode_tlb(&candidate_tlb,p);
    ram_size=get_u32(p); p+=4;
    if(ram_size!=memory->ram_size ||
       payload!=EE_SNAPSHOT_BYTES+TLB_SNAPSHOT_BYTES+4u+(size_t)ram_size) return -7;
    if(candidate_state.gpr[0]!=0u || candidate_state.in_delay_slot>1u) return -8;
    if(!snapshot_tlb_valid(&candidate_tlb)) return -9;
    /* Commit only after the entire payload and every TLB slot are validated. */
    memcpy(memory->ram,p,ram_size);
    *state=candidate_state; *tlb=candidate_tlb;
    return 0;
}
