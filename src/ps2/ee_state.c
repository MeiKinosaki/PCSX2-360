#include "ee_state.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

void ps2_ee_reset(ps2_ee_state_t *state)
{
    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->pc = UINT32_C(0xBFC00000);
    state->next_pc = state->pc + UINT32_C(4);
    state->cp0_random = UINT32_C(47);
    state->gpr[0] = 0;
}

void ps2_ee_raise_exception(ps2_ee_state_t *state, uint32_t fault_pc,
                            uint32_t exc_code, int in_delay_slot)
{
    uint32_t vector_base, vector_offset = UINT32_C(0x180);
    int already_exl;
    if (state == NULL) return;
    already_exl = (state->status & UINT32_C(0x2)) != 0u;
    if (exc_code == PS2_EE_EXC_TLBL || exc_code == PS2_EE_EXC_TLBS ||
        exc_code == PS2_EE_EXC_MOD) {
        /* Context.BadVPN2 and EntryHi.VPN2 are updated by address faults;
           preserve the software-managed PTEBase and ASID fields. */
        state->badvaddr = fault_pc;
        state->cp0_context = (state->cp0_context & UINT32_C(0xff800000)) |
                             ((fault_pc >> 9) & UINT32_C(0x007ffff0));
        state->cp0_entry_hi = (state->cp0_entry_hi & UINT32_C(0x000000ff)) |
                              (fault_pc & UINT32_C(0xffffe000));
    }
    if (!already_exl) {
        state->epc = in_delay_slot ? fault_pc - UINT32_C(4) : fault_pc;
        state->cause &= ~UINT32_C(0x8000007c);
        state->cause |= (exc_code & UINT32_C(0x1f)) << 2;
        if (in_delay_slot) state->cause |= UINT32_C(0x80000000);
        else state->cause &= ~UINT32_C(0x80000000);
        /* R5900 TLB refill exceptions use the refill vector; nested
           exceptions while EXL is set use the general exception vector. */
        if (exc_code == PS2_EE_EXC_TLBL || exc_code == PS2_EE_EXC_TLBS)
            vector_offset = 0u;
    }
    state->status |= UINT32_C(0x2);
    vector_base = (state->status & UINT32_C(0x00400000)) != 0u
                ? UINT32_C(0xBFC00200) : UINT32_C(0x80000000);
    state->pc = vector_base + vector_offset;
    state->next_pc = state->pc + UINT32_C(4);
    state->gpr[0] = 0;
}

int ps2_ee_cp0_read(const ps2_ee_state_t *state, unsigned reg, unsigned sel, uint32_t *value)
{
    if (state == NULL || value == NULL || sel != 0u) return -1;
    switch (reg) {
    case 0u:  *value = state->cp0_index; return 0;
    case 1u:  *value = state->cp0_random; return 0;
    case 2u:  *value = state->cp0_entry_lo0; return 0;
    case 3u:  *value = state->cp0_entry_lo1; return 0;
    case 4u:  *value = state->cp0_context; return 0;
    case 5u:  *value = state->cp0_page_mask; return 0;
    case 6u:  *value = state->cp0_wired; return 0;
    case 8u:  *value = state->badvaddr; return 0;
    case 10u: *value = state->cp0_entry_hi; return 0;
    case 12u: *value = state->status; return 0;
    case 13u: *value = state->cause; return 0;
    case 14u: *value = state->epc; return 0;
    default: return -2;
    }
}

int ps2_ee_cp0_write(ps2_ee_state_t *state, unsigned reg, unsigned sel, uint32_t value)
{
    if (state == NULL || sel != 0u) return -1;
    switch (reg) {
    case 0u:
        state->cp0_index = value & UINT32_C(0x8000003f);
        return 0;
    case 1u: return -3; /* Random is read-only in this interface. */
    case 2u:
        state->cp0_entry_lo0 = value & UINT32_C(0x03ffffff);
        return 0;
    case 3u:
        state->cp0_entry_lo1 = value & UINT32_C(0x03ffffff);
        return 0;
    case 4u:
        state->cp0_context = (state->cp0_context & UINT32_C(0xff800000)) |
                             (value & UINT32_C(0x007ffff0));
        return 0;
    case 5u:
        state->cp0_page_mask = value & UINT32_C(0x01ffe000);
        return 0;
    case 6u:
        state->cp0_wired = value & UINT32_C(0x3f);
        state->cp0_random = UINT32_C(47);
        return 0;
    case 10u:
        state->cp0_entry_hi = value & UINT32_C(0xffffe0ff);
        return 0;
    case 12u:
        /* This prototype models IE, EXL, ERL and BEV only; other Status bits
         * remain untouched until interrupt-mask and privilege logic exists. */
        state->status = (state->status & ~UINT32_C(0x00400007)) |
                        (value & UINT32_C(0x00400007));
        return 0;
    case 13u:
        /* Software interrupt pending bits are writable; ExcCode/BD are not. */
        state->cause = (state->cause & ~UINT32_C(0x00000300)) |
                       (value & UINT32_C(0x00000300));
        return 0;
    case 14u: state->epc = value; return 0;
    case 8u:  return -3; /* BadVAddr is read-only. */
    default: return -2;
    }
}

int ps2_ee_add32(ps2_ee_state_t *state, unsigned rd,
                 unsigned rs, unsigned rt)
{
    int64_t a, b, sum;
    int32_t result;

    if (state == NULL || rd >= PS2_EE_GPR_COUNT ||
        rs >= PS2_EE_GPR_COUNT || rt >= PS2_EE_GPR_COUNT) {
        return -1;
    }

    a = (int64_t)(int32_t)(uint32_t)state->gpr[rs];
    b = (int64_t)(int32_t)(uint32_t)state->gpr[rt];
    sum = a + b;
    if (sum > INT32_MAX || sum < INT32_MIN) {
        return 1;
    }

    result = (int32_t)sum;
    if (rd != 0u) {
        state->gpr[rd] = (uint64_t)(int64_t)result;
    }
    state->gpr[0] = 0;
    return 0;
}

int ps2_ee_sub32(ps2_ee_state_t *state, unsigned rd,
                 unsigned rs, unsigned rt)
{
    int64_t a, b, difference;
    int32_t result;

    if (state == NULL || rd >= PS2_EE_GPR_COUNT ||
        rs >= PS2_EE_GPR_COUNT || rt >= PS2_EE_GPR_COUNT) {
        return -1;
    }

    a = (int64_t)(int32_t)(uint32_t)state->gpr[rs];
    b = (int64_t)(int32_t)(uint32_t)state->gpr[rt];
    difference = a - b;
    if (difference > INT32_MAX || difference < INT32_MIN) {
        return 1;
    }

    result = (int32_t)difference;
    if (rd != 0u) {
        state->gpr[rd] = (uint64_t)(int64_t)result;
    }
    state->gpr[0] = 0;
    return 0;
}
