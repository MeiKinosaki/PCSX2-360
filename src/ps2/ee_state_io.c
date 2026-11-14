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

int ps2_ee_state_save(const ps2_ee_state_t *state, uint8_t *out, size_t capacity, size_t *written)
{
    size_t payload, total;
    if(written) *written=0u;
    if(!state || !out || !written) return -1;
    payload=sizeof(*state); total=SNAPSHOT_HEADER_SIZE+payload;
    if(capacity<total) return -2;
    put_u32(out,SNAPSHOT_MAGIC_EE); put_u32(out+4,PS2_EE_SNAPSHOT_VERSION);
    put_u32(out+8,(uint32_t)payload);
    memcpy(out+SNAPSHOT_HEADER_SIZE,state,payload);
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
    if(payload!=sizeof(candidate) || size!=SNAPSHOT_HEADER_SIZE+(size_t)payload) return -4;
    if(checksum(data+SNAPSHOT_HEADER_SIZE,payload)!=get_u32(data+12)) return -5;
    memcpy(&candidate,data+SNAPSHOT_HEADER_SIZE,sizeof(candidate));
    if(candidate.gpr[0]!=0u) return -6;
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
    if(memory->ram_size>UINT32_MAX ||
       memory->ram_size>SIZE_MAX-sizeof(*state)-sizeof(*tlb)-4u-SNAPSHOT_HEADER_SIZE) return -2;
    payload=sizeof(*state)+sizeof(*tlb)+4u+memory->ram_size;
    total=SNAPSHOT_HEADER_SIZE+payload;
    if(capacity<total) return -3;
    put_u32(out,SNAPSHOT_MAGIC_MACHINE); put_u32(out+4,1u); put_u32(out+8,(uint32_t)payload);
    p=out+SNAPSHOT_HEADER_SIZE;
    memcpy(p,state,sizeof(*state)); p+=sizeof(*state);
    memcpy(p,tlb,sizeof(*tlb)); p+=sizeof(*tlb);
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
    if(get_u32(data+4)!=1u) return -3;
    payload=get_u32(data+8);
    if(size!=SNAPSHOT_HEADER_SIZE+(size_t)payload) return -4;
    if(checksum(data+SNAPSHOT_HEADER_SIZE,payload)!=get_u32(data+12)) return -5;
    if(payload<sizeof(candidate_state)+sizeof(candidate_tlb)+4u) return -6;
    p=data+SNAPSHOT_HEADER_SIZE;
    memcpy(&candidate_state,p,sizeof(candidate_state)); p+=sizeof(candidate_state);
    memcpy(&candidate_tlb,p,sizeof(candidate_tlb)); p+=sizeof(candidate_tlb);
    ram_size=get_u32(p); p+=4;
    if(ram_size!=memory->ram_size ||
       payload!=sizeof(candidate_state)+sizeof(candidate_tlb)+4u+(size_t)ram_size) return -7;
    if(candidate_state.gpr[0]!=0u) return -8;
    memcpy(memory->ram,p,ram_size);
    *state=candidate_state; *tlb=candidate_tlb;
    return 0;
}
