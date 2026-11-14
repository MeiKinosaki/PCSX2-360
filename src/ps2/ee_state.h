#ifndef PS2PALACE360_EE_STATE_H
#define PS2PALACE360_EE_STATE_H

#include <stdint.h>

#define PS2_EE_GPR_COUNT 32u

/* MIPS-style Cause.ExcCode values used by the experimental EE core. */
#define PS2_EE_EXC_INT   0u
#define PS2_EE_EXC_MOD   1u
#define PS2_EE_EXC_TLBL  2u
#define PS2_EE_EXC_TLBS  3u
#define PS2_EE_EXC_ADEL  4u
#define PS2_EE_EXC_ADES  5u
#define PS2_EE_EXC_IBE   6u
#define PS2_EE_EXC_DBE   7u
#define PS2_EE_EXC_SYS   8u
#define PS2_EE_EXC_BP    9u
#define PS2_EE_EXC_RI    10u
#define PS2_EE_EXC_CPU   11u
#define PS2_EE_EXC_OV    12u
#define PS2_EE_EXC_TR    13u
#define PS2_EE_EXC_FPE   15u

typedef struct ps2_ee_state {
    uint64_t gpr[PS2_EE_GPR_COUNT];
    uint64_t hi;
    uint64_t lo;
    uint32_t pc;
    uint32_t next_pc;
    uint32_t branch_pc; /* Address of the control-transfer instruction owning the delay slot. */
    uint8_t in_delay_slot;
    uint32_t cause;
    uint32_t status;
    uint32_t epc;
    uint32_t badvaddr; /* CP0.BadVAddr shadow for TLB/address faults. */
    uint32_t cp0_context; /* CP0.Context: PTEBase plus BadVPN2. */
    /* CP0 TLB-management registers used by the stateful COP0/TLB execution path. */
    uint32_t cp0_index;
    uint32_t cp0_random;
    uint32_t cp0_entry_lo0;
    uint32_t cp0_entry_lo1;
    uint32_t cp0_page_mask;
    uint32_t cp0_wired;
    uint32_t cp0_entry_hi;
} ps2_ee_state_t;

/* Initial MIPS-style exception entry; exc_code is the Cause.ExcCode value. */
void ps2_ee_raise_exception(ps2_ee_state_t *state, uint32_t fault_pc,
                            uint32_t exc_code, int in_delay_slot);
/* Explicitly select the TLB refill vector only for a genuine translation miss. */
void ps2_ee_raise_exception_ex(ps2_ee_state_t *state, uint32_t fault_pc,
                               uint32_t exc_code, int in_delay_slot,
                               int tlb_refill);
void ps2_ee_reset(ps2_ee_state_t *state);
/* Partial CP0 register interface: only implemented architectural registers are accepted. */
int ps2_ee_cp0_read(const ps2_ee_state_t *state, unsigned reg, unsigned sel, uint32_t *value);
int ps2_ee_cp0_write(ps2_ee_state_t *state, unsigned reg, unsigned sel, uint32_t value);
int ps2_ee_add32(ps2_ee_state_t *state, unsigned rd,
                 unsigned rs, unsigned rt);
int ps2_ee_sub32(ps2_ee_state_t *state, unsigned rd,
                 unsigned rs, unsigned rt);

#endif
