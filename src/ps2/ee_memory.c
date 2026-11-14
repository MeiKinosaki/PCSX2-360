#include "ee_memory.h"
#include "ee_state.h"

#include <string.h>

int ps2_ee_translate_address_ex(uint32_t virtual_address, uint32_t *physical_address)
{
    uint32_t segment = virtual_address & UINT32_C(0xE0000000);
    if (physical_address == 0) return PS2_EE_TRANSLATE_ERR_ARGUMENT;
    /* KSEG0/KSEG1 are direct-mapped in this prototype only. */
    if (segment == UINT32_C(0x80000000) || segment == UINT32_C(0xA0000000)) {
        *physical_address = virtual_address & UINT32_C(0x1FFFFFFF);
        return PS2_EE_TRANSLATE_OK;
    }
    /* KUSEG/KSEG2 require a real TLB and are deliberately unsupported. */
    return PS2_EE_TRANSLATE_ERR_UNSUPPORTED;
}

/* Compatibility wrapper: preserve the original 0/-1 contract. */
int ps2_ee_translate_address(uint32_t virtual_address, uint32_t *physical_address)
{
    return ps2_ee_translate_address_ex(virtual_address, physical_address) ==
           PS2_EE_TRANSLATE_OK ? 0 : -1;
}

static int valid(const ps2_ee_memory_t *m, uint32_t address,
                 unsigned width)
{
    if (m == 0 || m->ram == 0) return 0;
    if (width != 1u && width != 2u && width != 4u && width != 8u) return 0;
    if ((address & (width - 1u)) != 0u) return 0;
    /* Subtraction form avoids overflow in address + width. */
    if ((uint64_t)address > (uint64_t)m->ram_size) return 0;
    if ((uint64_t)width > (uint64_t)m->ram_size - (uint64_t)address) return 0;
    return 1;
}

int ps2_ee_mem_load(ps2_ee_memory_t *m, uint32_t address,
                    unsigned width, uint64_t *value)
{
    uint64_t result = 0;
    unsigned i;
    if (value == 0 || !valid(m, address, width)) return -1;
    for (i = 0; i < width; ++i)
        result |= (uint64_t)m->ram[address + i] << (8u * i);
    *value = result;
    return 0;
}

int ps2_ee_mem_store(ps2_ee_memory_t *m, uint32_t address,
                     unsigned width, uint64_t value)
{
    unsigned i;
    if (!valid(m, address, width)) return -1;
    for (i = 0; i < width; ++i)
        m->ram[address + i] = (uint8_t)(value >> (8u * i));
    return 0;
}

int ps2_ee_mem_load_virtual_ex(ps2_ee_memory_t *m, uint32_t virtual_address,
                               unsigned width, uint64_t *value)
{
    uint32_t physical_address;
    int status;
    if (value == 0 || m == 0 || m->ram == 0)
        return PS2_EE_MEM_ERR_ARGUMENT;
    status = ps2_ee_translate_address_ex(virtual_address, &physical_address);
    if (status != PS2_EE_TRANSLATE_OK)
        return PS2_EE_MEM_ERR_TRANSLATION;
    return ps2_ee_mem_load(m, physical_address, width, value) == 0
           ? PS2_EE_MEM_OK : PS2_EE_MEM_ERR_ACCESS;
}

int ps2_ee_mem_store_virtual_ex(ps2_ee_memory_t *m, uint32_t virtual_address,
                                unsigned width, uint64_t value)
{
    uint32_t physical_address;
    int status;
    if (m == 0 || m->ram == 0)
        return PS2_EE_MEM_ERR_ARGUMENT;
    status = ps2_ee_translate_address_ex(virtual_address, &physical_address);
    if (status != PS2_EE_TRANSLATE_OK)
        return PS2_EE_MEM_ERR_TRANSLATION;
    return ps2_ee_mem_store(m, physical_address, width, value) == 0
           ? PS2_EE_MEM_OK : PS2_EE_MEM_ERR_ACCESS;
}

/* Compatibility wrappers preserve the original 0/-1 behavior. */
int ps2_ee_mem_load_virtual(ps2_ee_memory_t *m, uint32_t virtual_address,
                            unsigned width, uint64_t *value)
{
    return ps2_ee_mem_load_virtual_ex(m, virtual_address, width, value) ==
           PS2_EE_MEM_OK ? 0 : -1;
}

int ps2_ee_mem_store_virtual(ps2_ee_memory_t *m, uint32_t virtual_address,
                             unsigned width, uint64_t value)
{
    return ps2_ee_mem_store_virtual_ex(m, virtual_address, width, value) ==
           PS2_EE_MEM_OK ? 0 : -1;
}

/* v0.101b: small, explicit TLB scaffold for 4 KiB pages. This is not yet
 * a complete R5900 MMU: page masks, CP0 registers, privilege checks and
 * architectural exception entry are handled by later work. */
void ps2_ee_tlb_reset(ps2_ee_tlb_t *tlb)
{
    unsigned i;
    if (tlb == 0) return;
    tlb->current_asid = 0;
    for (i = 0; i < PS2_EE_TLB_ENTRY_COUNT; ++i) {
        tlb->entries[i].virtual_page = 0;
        tlb->entries[i].physical_page = 0;
        tlb->entries[i].asid = 0;
        tlb->entries[i].valid = 0;
        tlb->entries[i].dirty = 0;
        tlb->entries[i].global = 0;
        tlb->entries[i].present = 0;
    }
}

int ps2_ee_tlb_write(ps2_ee_tlb_t *tlb, unsigned index,
                     uint32_t virtual_page, uint32_t physical_page,
                     uint8_t asid, int valid_entry, int dirty, int global)
{
    ps2_ee_tlb_entry_t *e;
    if (tlb == 0 || index >= PS2_EE_TLB_ENTRY_COUNT ||
        (virtual_page & (PS2_EE_TLB_PAGE_SIZE - 1u)) != 0u ||
        (physical_page & (PS2_EE_TLB_PAGE_SIZE - 1u)) != 0u)
        return PS2_EE_TLB_ERR_ARGUMENT;
    e = &tlb->entries[index];
    e->virtual_page = virtual_page;
    e->physical_page = physical_page;
    e->asid = asid;
    e->valid = valid_entry != 0;
    e->dirty = dirty != 0;
    e->global = global != 0;
    e->present = 1;
    return PS2_EE_TLB_OK;
}

int ps2_ee_tlb_lookup(const ps2_ee_tlb_t *tlb, uint32_t virtual_address,
                      ps2_ee_tlb_entry_t *entry)
{
    uint32_t page;
    const ps2_ee_tlb_entry_t *match = 0;
    unsigned i, matches = 0;
    if (tlb == 0 || entry == 0) return PS2_EE_TLB_ERR_ARGUMENT;
    page = virtual_address & ~(uint32_t)(PS2_EE_TLB_PAGE_SIZE - 1u);
    for (i = 0; i < PS2_EE_TLB_ENTRY_COUNT; ++i) {
        const ps2_ee_tlb_entry_t *e = &tlb->entries[i];
        if (!e->valid || e->virtual_page != page ||
            (!e->global && e->asid != tlb->current_asid))
            continue;
        match = e;
        ++matches;
    }
    /* Ignore invalid entries and never silently choose among duplicate hits. */
    if (matches > 1u) return PS2_EE_TLB_ERR_MULTIPLE;
    if (matches == 0u) return PS2_EE_TLB_ERR_MISS;
    *entry = *match;
    return PS2_EE_TLB_OK;
}

/* TLBP-like search over programmed slots. The valid/dirty permission
 * bits do not participate in tag matching; a programmed invalid mapping
 * is still discoverable by the probe operation. */
int ps2_ee_tlb_probe(const ps2_ee_tlb_t *tlb, uint32_t virtual_address,
                     unsigned *index)
{
    uint32_t page;
    unsigned i, matches = 0, found = 0;
    if (tlb == 0 || index == 0) return PS2_EE_TLB_ERR_ARGUMENT;
    page = virtual_address & ~(uint32_t)(PS2_EE_TLB_PAGE_SIZE - 1u);
    for (i = 0; i < PS2_EE_TLB_ENTRY_COUNT; ++i) {
        const ps2_ee_tlb_entry_t *e = &tlb->entries[i];
        if (!e->present || e->virtual_page != page ||
            (!e->global && e->asid != tlb->current_asid))
            continue;
        found = i;
        ++matches;
    }
    if (matches > 1u) return PS2_EE_TLB_ERR_MULTIPLE;
    if (matches == 0u) return PS2_EE_TLB_ERR_MISS;
    *index = found;
    return PS2_EE_TLB_OK;
}

int ps2_ee_tlb_translate(const ps2_ee_tlb_t *tlb, uint32_t virtual_address,
                         int is_write, uint32_t *physical_address)
{
    uint32_t page, offset, resolved = 0;
    const ps2_ee_tlb_entry_t *match = 0;
    unsigned i, matches = 0;
    if (tlb == 0 || physical_address == 0)
        return PS2_EE_TLB_ERR_ARGUMENT;
    page = virtual_address & ~(uint32_t)(PS2_EE_TLB_PAGE_SIZE - 1u);
    offset = virtual_address & (PS2_EE_TLB_PAGE_SIZE - 1u);
    for (i = 0; i < PS2_EE_TLB_ENTRY_COUNT; ++i) {
        const ps2_ee_tlb_entry_t *e = &tlb->entries[i];
        if (!e->valid || e->virtual_page != page ||
            (!e->global && e->asid != tlb->current_asid))
            continue;
        match = e;
        ++matches;
    }
    /* Do not silently pick the first mapping when valid entries collide. */
    if (matches > 1u) return PS2_EE_TLB_ERR_MULTIPLE;
    if (matches == 0u) {
        /* Preserve the prototype's invalid-entry signal if a matching but
         * invalid entry exists and no valid mapping supersedes it. */
        for (i = 0; i < PS2_EE_TLB_ENTRY_COUNT; ++i) {
            const ps2_ee_tlb_entry_t *e = &tlb->entries[i];
            if (e->present && e->virtual_page == page &&
                (e->global || e->asid == tlb->current_asid))
                return PS2_EE_TLB_ERR_INVALID;
        }
        return PS2_EE_TLB_ERR_MISS;
    }
    if (is_write && !match->dirty) return PS2_EE_TLB_ERR_MODIFIED;
    resolved = match->physical_page | offset;
    *physical_address = resolved;
    return PS2_EE_TLB_OK;
}


/* v0.102b: small management helpers for the experimental TLB table. */
int ps2_ee_tlb_set_asid(ps2_ee_tlb_t *tlb, uint8_t asid)
{
    if (tlb == 0) return PS2_EE_TLB_ERR_ARGUMENT;
    tlb->current_asid = asid;
    return PS2_EE_TLB_OK;
}

int ps2_ee_tlb_read(const ps2_ee_tlb_t *tlb, unsigned index,
                    ps2_ee_tlb_entry_t *entry)
{
    if (tlb == 0 || entry == 0 || index >= PS2_EE_TLB_ENTRY_COUNT)
        return PS2_EE_TLB_ERR_ARGUMENT;
    *entry = tlb->entries[index];
    return PS2_EE_TLB_OK;
}

int ps2_ee_tlb_invalidate_asid(ps2_ee_tlb_t *tlb, uint8_t asid)
{
    unsigned i;
    if (tlb == 0) return PS2_EE_TLB_ERR_ARGUMENT;
    for (i = 0; i < PS2_EE_TLB_ENTRY_COUNT; ++i) {
        ps2_ee_tlb_entry_t *e = &tlb->entries[i];
        /* Global mappings are independent of the current ASID. */
        if (!e->global && e->asid == asid) e->valid = 0;
    }
    return PS2_EE_TLB_OK;
}


/* v0.103b: combine direct-mapped segments with the experimental TLB.
 * No CP0 exception entry or privilege-mode enforcement is performed here. */
int ps2_ee_translate_address_with_tlb(const ps2_ee_tlb_t *tlb,
                                      uint32_t virtual_address, int is_write,
                                      uint32_t *physical_address)
{
    uint32_t segment;
    if (physical_address == 0) return PS2_EE_TLB_ERR_ARGUMENT;
    segment = virtual_address & UINT32_C(0xE0000000);
    if (segment == UINT32_C(0x80000000) || segment == UINT32_C(0xA0000000)) {
        *physical_address = virtual_address & UINT32_C(0x1FFFFFFF);
        return PS2_EE_TLB_OK;
    }
    return ps2_ee_tlb_translate(tlb, virtual_address, is_write, physical_address);
}


/* v0.104b: memory access routed through direct-map/TLB dispatcher. */
int ps2_ee_mem_load_virtual_tlb(ps2_ee_memory_t *m, const ps2_ee_tlb_t *tlb,
                                uint32_t virtual_address, unsigned width,
                                uint64_t *value)
{
    uint32_t physical_address;
    int status;
    if (m == 0 || m->ram == 0 || value == 0) return PS2_EE_MEM_ERR_ARGUMENT;
    status = ps2_ee_translate_address_with_tlb(tlb, virtual_address, 0, &physical_address);
    if (status != PS2_EE_TLB_OK) return PS2_EE_MEM_ERR_TRANSLATION;
    return ps2_ee_mem_load(m, physical_address, width, value) == 0
           ? PS2_EE_MEM_OK : PS2_EE_MEM_ERR_ACCESS;
}

int ps2_ee_mem_store_virtual_tlb(ps2_ee_memory_t *m, const ps2_ee_tlb_t *tlb,
                                 uint32_t virtual_address, unsigned width,
                                 uint64_t value)
{
    uint32_t physical_address;
    int status;
    if (m == 0 || m->ram == 0) return PS2_EE_MEM_ERR_ARGUMENT;
    status = ps2_ee_translate_address_with_tlb(tlb, virtual_address, 1, &physical_address);
    if (status != PS2_EE_TLB_OK) return PS2_EE_MEM_ERR_TRANSLATION;
    return ps2_ee_mem_store(m, physical_address, width, value) == 0
           ? PS2_EE_MEM_OK : PS2_EE_MEM_ERR_ACCESS;
}


/* v0.107b: permission/status-only check without forming a physical address. */
int ps2_ee_tlb_check_page(const ps2_ee_tlb_t *tlb, uint32_t virtual_address,
                          int is_write)
{
    ps2_ee_tlb_entry_t entry;
    int status = ps2_ee_tlb_lookup(tlb, virtual_address, &entry);
    if (status == PS2_EE_TLB_ERR_MISS) {
        uint32_t page;
        unsigned i;
        if (tlb == 0) return PS2_EE_TLB_ERR_ARGUMENT;
        page = virtual_address & ~(uint32_t)(PS2_EE_TLB_PAGE_SIZE - 1u);
        /* Distinguish a programmed invalid entry from a genuine miss.
         * Full invalidation clears metadata, so flushed slots won't match. */
        for (i = 0; i < PS2_EE_TLB_ENTRY_COUNT; ++i) {
            const ps2_ee_tlb_entry_t *e = &tlb->entries[i];
            if (e->present && !e->valid && e->virtual_page == page &&
                (e->global || e->asid == tlb->current_asid))
                return PS2_EE_TLB_ERR_INVALID;
        }
        return status;
    }
    if (status != PS2_EE_TLB_OK) return status;
    if (is_write && !entry.dirty) return PS2_EE_TLB_ERR_MODIFIED;
    return PS2_EE_TLB_OK;
}


/* v0.109b: diagnostic helper for the experimental table. */
unsigned ps2_ee_tlb_count_valid(const ps2_ee_tlb_t *tlb)
{
    unsigned i, count = 0;
    if (tlb == 0) return 0;
    for (i = 0; i < PS2_EE_TLB_ENTRY_COUNT; ++i)
        if (tlb->entries[i].valid) ++count;
    return count;
}


/* V2.0.1b: invalidate every entry, including global mappings. */
int ps2_ee_tlb_invalidate_all(ps2_ee_tlb_t *tlb)
{
    unsigned i;
    if (tlb == 0) return PS2_EE_TLB_ERR_ARGUMENT;
    /* Flush the entry metadata as well as the valid bit. Keeping a stale
     * virtual_page here makes translate() misreport an invalid entry after
     * a full flush instead of reporting a genuine TLB miss. */
    for (i = 0; i < PS2_EE_TLB_ENTRY_COUNT; ++i) {
        tlb->entries[i].virtual_page = 0;
        tlb->entries[i].physical_page = 0;
        tlb->entries[i].asid = 0;
        tlb->entries[i].valid = 0;
        tlb->entries[i].dirty = 0;
        tlb->entries[i].global = 0;
        tlb->entries[i].present = 0;
    }
    return PS2_EE_TLB_OK;
}


/* V2.0.2b: count valid entries matching a page under the active ASID.
 * Diagnostic only: it reports duplicates but does not emulate TLB shutdown. */
unsigned ps2_ee_tlb_count_matches(const ps2_ee_tlb_t *tlb, uint32_t virtual_address)
{
    uint32_t page;
    unsigned i, count = 0;
    if (tlb == 0) return 0;
    page = virtual_address & ~(uint32_t)(PS2_EE_TLB_PAGE_SIZE - 1u);
    for (i = 0; i < PS2_EE_TLB_ENTRY_COUNT; ++i) {
        const ps2_ee_tlb_entry_t *e = &tlb->entries[i];
        if (e->valid && e->virtual_page == page &&
            (e->global || e->asid == tlb->current_asid))
            ++count;
    }
    return count;
}

/* Architectural-shape R5900 TLB model. This is deliberately separate from
 * the earlier one-page-per-slot prototype so the 48 hardware slots each
 * hold an even/odd 4 KiB page pair. PageMask variants are not implemented. */
static ps2_ee_tlb_half_t decode_entry_lo(uint32_t entry_lo)
{
    ps2_ee_tlb_half_t half;
    half.physical_page = (entry_lo & UINT32_C(0x03ffffc0)) << 6;
    half.valid = (uint8_t)((entry_lo >> 1) & 1u);
    half.dirty = (uint8_t)((entry_lo >> 2) & 1u);
    half.global = (uint8_t)(entry_lo & 1u);
    return half;
}

void ps2_ee_tlb_arch_reset(ps2_ee_tlb_arch_t *tlb)
{
    if (tlb == 0) return;
    memset(tlb, 0, sizeof(*tlb));
}

/* PageMask follows the paired-page R5900 encoding. The mask field is
 * validated as contiguous pairs of 4 KiB pages; unsupported encodings fail
 * explicitly rather than silently aliasing addresses. */
static int tlb_page_mask_valid(uint32_t page_mask)
{
    uint32_t m = (page_mask & UINT32_C(0x01ffe000)) >> 13;
    if ((page_mask & ~UINT32_C(0x01ffe000)) != 0u) return 0;
    return (m & (m + 1u)) == 0u;
}

static uint32_t tlb_pair_compare_mask(uint32_t page_mask)
{
    return ~(page_mask | UINT32_C(0x1fff));
}

int ps2_ee_tlb_arch_write_indexed_masked(ps2_ee_tlb_arch_t *tlb, unsigned index,
                                         uint32_t entry_hi, uint32_t entry_lo0,
                                         uint32_t entry_lo1, uint32_t page_mask)
{
    ps2_ee_tlb_pair_entry_t *entry;
    uint32_t pair_mask;
    if (tlb == 0 || index >= 48u) return PS2_EE_TLB_ERR_ARGUMENT;
    if (!tlb_page_mask_valid(page_mask)) return PS2_EE_TLB_ERR_UNSUPPORTED;
    pair_mask = tlb_pair_compare_mask(page_mask);
    entry = &tlb->entries[index];
    entry->page_mask = page_mask;
    entry->vpn2 = entry_hi & pair_mask;
    entry->asid = (uint8_t)(entry_hi & UINT32_C(0xff));
    entry->even_page = decode_entry_lo(entry_lo0);
    entry->odd_page = decode_entry_lo(entry_lo1);
    entry->global = (uint8_t)(((entry_lo0 & 1u) != 0u) &&
                              ((entry_lo1 & 1u) != 0u));
    entry->present = 1;
    return PS2_EE_TLB_OK;
}

int ps2_ee_tlb_arch_write_indexed(ps2_ee_tlb_arch_t *tlb, unsigned index,
                                  uint32_t entry_hi, uint32_t entry_lo0,
                                  uint32_t entry_lo1)
{
    return ps2_ee_tlb_arch_write_indexed_masked(tlb, index, entry_hi,
                                                entry_lo0, entry_lo1, 0u);
}

int ps2_ee_tlb_arch_read_indexed_masked(const ps2_ee_tlb_arch_t *tlb, unsigned index,
                                        uint32_t *entry_hi, uint32_t *entry_lo0,
                                        uint32_t *entry_lo1, uint32_t *page_mask)
{
    const ps2_ee_tlb_pair_entry_t *entry;
    if (tlb == 0 || index >= 48u || entry_hi == 0 ||
        entry_lo0 == 0 || entry_lo1 == 0 || page_mask == 0)
        return PS2_EE_TLB_ERR_ARGUMENT;
    entry = &tlb->entries[index];
    if (!entry->present) return PS2_EE_TLB_ERR_MISS;
    *entry_hi = entry->vpn2 | entry->asid;
    *entry_lo0 = ((entry->even_page.physical_page >> 12) << 6) |
                 ((uint32_t)entry->even_page.dirty << 2) |
                 ((uint32_t)entry->even_page.valid << 1) |
                 (uint32_t)entry->even_page.global;
    *entry_lo1 = ((entry->odd_page.physical_page >> 12) << 6) |
                 ((uint32_t)entry->odd_page.dirty << 2) |
                 ((uint32_t)entry->odd_page.valid << 1) |
                 (uint32_t)entry->odd_page.global;
    *page_mask = entry->page_mask;
    return PS2_EE_TLB_OK;
}

int ps2_ee_tlb_arch_read_indexed(const ps2_ee_tlb_arch_t *tlb, unsigned index,
                                 uint32_t *entry_hi, uint32_t *entry_lo0,
                                 uint32_t *entry_lo1)
{
    uint32_t ignored_mask;
    return ps2_ee_tlb_arch_read_indexed_masked(tlb, index, entry_hi,
                                               entry_lo0, entry_lo1,
                                               &ignored_mask);
}

int ps2_ee_tlb_arch_probe(const ps2_ee_tlb_arch_t *tlb,
                          uint32_t virtual_address, unsigned *index)
{
    unsigned i, matches = 0, found = 0;
    if (tlb == 0 || index == 0) return PS2_EE_TLB_ERR_ARGUMENT;
    for (i = 0; i < 48u; ++i) {
        const ps2_ee_tlb_pair_entry_t *entry = &tlb->entries[i];
        uint32_t compare_mask;
        if (!entry->present) continue;
        compare_mask = tlb_pair_compare_mask(entry->page_mask);
        if ((entry->vpn2 & compare_mask) != (virtual_address & compare_mask) ||
            (!entry->global && entry->asid != tlb->current_asid))
            continue;
        found = i;
        ++matches;
    }
    if (matches > 1u) return PS2_EE_TLB_ERR_MULTIPLE;
    if (matches == 0u) return PS2_EE_TLB_ERR_MISS;
    *index = found;
    return PS2_EE_TLB_OK;
}

int ps2_ee_tlb_arch_translate(const ps2_ee_tlb_arch_t *tlb,
                              uint32_t virtual_address, int is_write,
                              uint32_t *physical_address)
{
    unsigned index;
    uint32_t pair_offset, page_size, page_offset, physical_base;
    const ps2_ee_tlb_pair_entry_t *entry;
    const ps2_ee_tlb_half_t *half;
    int status;
    if (physical_address == 0) return PS2_EE_TLB_ERR_ARGUMENT;
    status = ps2_ee_tlb_arch_probe(tlb, virtual_address, &index);
    if (status != PS2_EE_TLB_OK) return status;
    entry = &tlb->entries[index];
    page_size = ((entry->page_mask | UINT32_C(0x1fff)) + 1u) >> 1;
    if (page_size < UINT32_C(0x1000)) return PS2_EE_TLB_ERR_UNSUPPORTED;
    pair_offset = virtual_address & (page_size * 2u - 1u);
    half = (pair_offset & page_size) != 0u ? &entry->odd_page : &entry->even_page;
    if (!half->valid) return PS2_EE_TLB_ERR_INVALID;
    if (is_write && !half->dirty) return PS2_EE_TLB_ERR_MODIFIED;
    page_offset = pair_offset & (page_size - 1u);
    physical_base = half->physical_page & ~(page_size - 1u);
    *physical_address = physical_base | page_offset;
    return PS2_EE_TLB_OK;
}

/* Experimental access path using the architectural paired-page TLB.
 * This deliberately remains separate from the legacy fixed-page TLB API. */
int ps2_ee_mem_load_virtual_arch_tlb(ps2_ee_memory_t *m,
                                     const ps2_ee_tlb_arch_t *tlb,
                                     uint32_t virtual_address, unsigned width,
                                     uint64_t *value)
{
    uint32_t physical_address;
    int status;
    if (m == 0 || m->ram == 0 || value == 0)
        return PS2_EE_MEM_ERR_ARGUMENT;
    status = ps2_ee_tlb_arch_translate(tlb, virtual_address, 0, &physical_address);
    if (status != PS2_EE_TLB_OK) return PS2_EE_MEM_ERR_TRANSLATION;
    return ps2_ee_mem_load(m, physical_address, width, value) == 0
           ? PS2_EE_MEM_OK : PS2_EE_MEM_ERR_ACCESS;
}

int ps2_ee_mem_store_virtual_arch_tlb(ps2_ee_memory_t *m,
                                      const ps2_ee_tlb_arch_t *tlb,
                                      uint32_t virtual_address, unsigned width,
                                      uint64_t value)
{
    uint32_t physical_address;
    int status;
    if (m == 0 || m->ram == 0) return PS2_EE_MEM_ERR_ARGUMENT;
    status = ps2_ee_tlb_arch_translate(tlb, virtual_address, 1, &physical_address);
    if (status != PS2_EE_TLB_OK) return PS2_EE_MEM_ERR_TRANSLATION;
    return ps2_ee_mem_store(m, physical_address, width, value) == 0
           ? PS2_EE_MEM_OK : PS2_EE_MEM_ERR_ACCESS;
}

int ps2_ee_cp0_tlbwi(struct ps2_ee_state *opaque_state, ps2_ee_tlb_arch_t *tlb)
{
    ps2_ee_state_t *state = (ps2_ee_state_t *)opaque_state;
    unsigned index;
    if (state == 0 || tlb == 0) return PS2_EE_TLB_ERR_ARGUMENT;
    index = state->cp0_index & UINT32_C(0x3f);
    if (index >= 48u) return PS2_EE_TLB_ERR_ARGUMENT;
    return ps2_ee_tlb_arch_write_indexed_masked(tlb, index, state->cp0_entry_hi,
                                                state->cp0_entry_lo0,
                                                state->cp0_entry_lo1,
                                                state->cp0_page_mask);
}

int ps2_ee_cp0_tlbwr(struct ps2_ee_state *opaque_state, ps2_ee_tlb_arch_t *tlb)
{
    ps2_ee_state_t *state = (ps2_ee_state_t *)opaque_state;
    unsigned wired, random, status;
    if (state == 0 || tlb == 0) return PS2_EE_TLB_ERR_ARGUMENT;
    wired = state->cp0_wired & UINT32_C(0x3f);
    if (wired > 47u) wired = 47u;
    random = state->cp0_random & UINT32_C(0x3f);
    if (random < wired || random > 47u) random = 47u;
    status = ps2_ee_tlb_arch_write_indexed_masked(tlb, random, state->cp0_entry_hi,
                                                  state->cp0_entry_lo0,
                                                  state->cp0_entry_lo1,
                                                  state->cp0_page_mask);
    if (status != PS2_EE_TLB_OK) return status;
    state->cp0_random = random == wired ? 47u : random - 1u;
    return PS2_EE_TLB_OK;
}

int ps2_ee_cp0_tlbr(struct ps2_ee_state *opaque_state, const ps2_ee_tlb_arch_t *tlb)
{
    ps2_ee_state_t *state = (ps2_ee_state_t *)opaque_state;
    unsigned index;
    int status;
    uint32_t hi, lo0, lo1, page_mask;
    if (state == 0 || tlb == 0) return PS2_EE_TLB_ERR_ARGUMENT;
    index = state->cp0_index & UINT32_C(0x3f);
    if (index >= 48u) return PS2_EE_TLB_ERR_ARGUMENT;
    status = ps2_ee_tlb_arch_read_indexed_masked(tlb, index, &hi, &lo0, &lo1, &page_mask);
    if (status != PS2_EE_TLB_OK) return status;
    state->cp0_entry_hi = hi;
    state->cp0_entry_lo0 = lo0;
    state->cp0_entry_lo1 = lo1;
    state->cp0_page_mask = page_mask;
    return PS2_EE_TLB_OK;
}

int ps2_ee_cp0_tlbp(struct ps2_ee_state *opaque_state, const ps2_ee_tlb_arch_t *tlb)
{
    ps2_ee_state_t *state = (ps2_ee_state_t *)opaque_state;
    ps2_ee_tlb_arch_t lookup;
    unsigned index;
    int status;
    if (state == 0 || tlb == 0) return PS2_EE_TLB_ERR_ARGUMENT;
    /* EntryHi supplies both the VPN and ASID; don't rely on a stale external ASID. */
    lookup = *tlb;
    lookup.current_asid = (uint8_t)(state->cp0_entry_hi & UINT32_C(0xff));
    status = ps2_ee_tlb_arch_probe(&lookup, state->cp0_entry_hi & UINT32_C(0xffffe000),
                                   &index);
    if (status == PS2_EE_TLB_OK) {
        state->cp0_index = index;
        return PS2_EE_TLB_OK;
    }
    if (status == PS2_EE_TLB_ERR_MISS) {
        state->cp0_index = UINT32_C(0x80000000);
    }
    return status;
}
