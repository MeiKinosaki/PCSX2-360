#ifndef PCSX2_360_IOP_H
#define PCSX2_360_IOP_H
#include <stdint.h>
typedef struct ps2_iop_state {
    uint32_t gpr[32], hi, lo, pc, next_pc;
    uint64_t cycle_count;
    uint32_t badvaddr;
    uint8_t halted;
} ps2_iop_state_t;
typedef int (*ps2_iop_read32_fn)(void *opaque,uint32_t address,uint32_t *value);
typedef int (*ps2_iop_write32_fn)(void *opaque,uint32_t address,uint32_t value);
typedef struct ps2_iop_bus { ps2_iop_read32_fn read32; ps2_iop_write32_fn write32; void *opaque; } ps2_iop_bus_t;
void ps2_iop_reset(ps2_iop_state_t *iop,uint32_t reset_pc);
int ps2_iop_step(ps2_iop_state_t *iop,const ps2_iop_bus_t *bus);
#endif
