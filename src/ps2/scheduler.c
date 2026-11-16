#include "scheduler.h"
#include <stddef.h>
void ps2_scheduler_reset(ps2_scheduler_t *s)
{
    if(s==0)return;
    s->master_ticks=0;s->iop_divider=0;s->stopped=0;
}
int ps2_scheduler_run(ps2_scheduler_t *s,ps2_ee_state_t *ee,
                      ps2_iop_state_t *iop,const ps2_iop_bus_t *iop_bus,
                      ps2_dma_controller_t *dma,ps2_dma_fabric_t *fabric,
                      ps2_ee_bus_t *bus,uint32_t cycles)
{
    uint32_t n;
    if(s==0 || ee==0 || iop==0 || iop_bus==0 || dma==0 || fabric==0 || bus==0)return -1;
    if(s->stopped)return 0;
    for(n=0;n<cycles;n++){
        unsigned channel=0;
        int rc;
        ++s->master_ticks;++ee->cycle_count;
        if(++s->iop_divider>=8u){
            s->iop_divider=0;
            rc=ps2_iop_step(iop,iop_bus);
            if(rc<0)return rc;
        }
        rc=ps2_dma_fabric_step_bus(fabric,dma,bus,&channel);
        if(rc<0)return rc;
        if(rc==1 && ps2_dma_fabric_take_completion(fabric,channel)==1)
            (void)ps2_dma_complete_channel(dma,channel,ee);
        /* Keep EE IP2 asserted while either DICR bank still requests service. */
        ps2_dma_sync_irq(dma,ee);
    }
    return 0;
}
