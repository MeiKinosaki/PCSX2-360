#include "scheduler.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct mem { uint32_t words[16]; } mem_t;
typedef struct dev { unsigned calls; } dev_t;
static int read32(void *o,uint32_t a,uint32_t *v){mem_t*m=o;if((a&3u)||a/4u>=16u)return -1;*v=m->words[a/4u];return 0;}
static int write32(void *o,uint32_t a,uint32_t v){mem_t*m=o;if((a&3u)||a/4u>=16u)return -1;m->words[a/4u]=v;return 0;}
static int device(void *o,unsigned ch,uint8_t q[16],int dir){dev_t*d=o;(void)ch;(void)q;(void)dir;++d->calls;return 0;}
int main(void)
{
    ps2_scheduler_t sched;ps2_ee_state_t ee;ps2_iop_state_t iop;
    ps2_dma_controller_t dma;ps2_dma_fabric_t fabric;ps2_ee_bus_t bus;
    uint8_t ram[128]={0},scratch[PS2_EE_SCRATCHPAD_BYTES];mem_t imem;
    dev_t dev={0};ps2_iop_bus_t ibus={read32,write32,&imem};
    memset(&imem,0,sizeof imem);imem.words[0]=UINT32_C(0x24010007);
    ps2_ee_reset(&ee);ps2_iop_reset(&iop,0);ps2_dma_reset(&dma);
    ps2_dma_fabric_reset(&fabric);ps2_scheduler_reset(&sched);
    bus.ram=ram;bus.ram_size=sizeof ram;bus.scratchpad=scratch;bus.scratchpad_size=sizeof scratch;
    assert(ps2_dma_fabric_bind(&fabric,PS2_DMA_ENDPOINT_SIF0,device,&dev)==0);
    assert(ps2_dma_set_channel_priority(&dma,5u,2u,1)==0);
    assert(ps2_dma_write_dicr(&dma,0u,(1u<<24)|(1u<<13))==0);
    dma.channel[5].chcr=PS2_DMA_CHCR_STR|PS2_DMA_CHCR_DIR;
    dma.channel[5].madr=0u;dma.channel[5].qwc=1u;
    assert(ps2_scheduler_run(&sched,&ee,&iop,&ibus,&dma,&fabric,&bus,8u)==0);
    assert(sched.master_ticks==8u && ee.cycle_count==8u);
    assert(iop.cycle_count==1u && iop.gpr[1]==7u);
    assert(dev.calls==1u && dma.channel[5].qwc==0u);
    assert((ee.cause&(1u<<10))!=0u);

    /* Disabled DPCR channel must remain pending without touching the endpoint. */
    ps2_ee_reset(&ee); ps2_iop_reset(&iop,0); ps2_dma_reset(&dma);
    ps2_dma_fabric_reset(&fabric); ps2_scheduler_reset(&sched);
    dev.calls=0u;
    assert(ps2_dma_fabric_bind(&fabric,PS2_DMA_ENDPOINT_SIF0,device,&dev)==0);
    dma.channel[5].chcr=PS2_DMA_CHCR_STR|PS2_DMA_CHCR_DIR;
    dma.channel[5].madr=0u; dma.channel[5].qwc=1u;
    assert(ps2_scheduler_run(&sched,&ee,&iop,&ibus,&dma,&fabric,&bus,1u)==0);
    assert(dev.calls==0u && dma.channel[5].qwc==1u);
    assert((dma.channel[5].chcr&PS2_DMA_CHCR_STR)!=0u);
    assert((ee.cause&(1u<<10))==0u);

    /* A physical-bus fault must stop STR and must not signal completion IRQ. */
    assert(ps2_dma_set_channel_priority(&dma,5u,2u,1)==0);
    dma.channel[5].madr=UINT32_C(0x1000);
    assert(ps2_scheduler_run(&sched,&ee,&iop,&ibus,&dma,&fabric,&bus,1u)==-5);
    assert((dma.channel[5].chcr&PS2_DMA_CHCR_STR)==0u);
    assert(ps2_dma_fabric_take_completion(&fabric,5u)==0);
    assert((ee.cause&(1u<<10))==0u);
    puts("OK: scheduler DMA completion, DPCR gating and bus-fault recovery tests.");
    return 0;
}
