#ifndef PCSX2_360_SCHEDULER_H
#define PCSX2_360_SCHEDULER_H
#include "dma_endpoints.h"
#include "ee_bus.h"
#include "ee_state.h"
#include "iop.h"
#include <stdint.h>
typedef struct ps2_scheduler {
    uint64_t master_ticks;
    uint32_t iop_divider;
    uint8_t stopped;
} ps2_scheduler_t;
void ps2_scheduler_reset(ps2_scheduler_t *scheduler);
int ps2_scheduler_run(ps2_scheduler_t *scheduler,ps2_ee_state_t *ee,
                      ps2_iop_state_t *iop,const ps2_iop_bus_t *iop_bus,
                      ps2_dma_controller_t *dma,ps2_dma_fabric_t *fabric,
                      ps2_ee_bus_t *ee_bus,uint32_t ee_cycles);
#endif
