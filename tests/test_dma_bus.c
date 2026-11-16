#include "dma_endpoints.h"
#include "ee_bus.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct dev { unsigned calls; uint8_t last[16]; } dev_t;
static void write_tag(uint8_t *p,uint16_t qwc,unsigned id,uint32_t addr)
{
    uint64_t lo=(uint64_t)qwc|((uint64_t)id<<28)|((uint64_t)addr<<32);
    unsigned i; memset(p,0,16u);
    for(i=0;i<8u;i++)p[i]=(uint8_t)(lo>>(8u*i));
}
static int transfer(void *opaque,unsigned channel,uint8_t qword[16],int direction)
{
    dev_t *d=(dev_t*)opaque; (void)channel; ++d->calls;
    if(direction) memcpy(d->last,qword,16u);
    else memset(qword,0xC3,16u);
    return 0;
}
int main(void)
{
    uint8_t ram[128]={0}, scratch[PS2_EE_SCRATCHPAD_BYTES];
    ps2_ee_bus_t bus={ram,sizeof ram,scratch,sizeof scratch};
    ps2_dma_controller_t dma; ps2_dma_fabric_t fabric;
    dev_t dev={0,{0}}; unsigned serviced=99u,i;
    ps2_dma_reset(&dma); ps2_dma_fabric_reset(&fabric);
    assert(ps2_dma_fabric_bind(&fabric,PS2_DMA_ENDPOINT_SIF0,transfer,&dev)==0);
    assert(ps2_dma_set_channel_priority(&dma,5u,2u,1)==0);
    for(i=0;i<16u;i++) ram[32u+i]=(uint8_t)(i+3u);
    dma.channel[5].chcr=PS2_DMA_CHCR_STR|PS2_DMA_CHCR_DIR;
    dma.channel[5].madr=32u; dma.channel[5].qwc=1u;
    assert(ps2_dma_fabric_step_bus(&fabric,&dma,&bus,&serviced)==1);
    assert(serviced==5u && dev.calls==1u && dev.last[0]==3u && dev.last[15]==18u);
    assert(ps2_dma_fabric_take_completion(&fabric,5u)==1);
    dma.channel[5].chcr=PS2_DMA_CHCR_STR; dma.channel[5].madr=PS2_EE_SCRATCHPAD_BASE;
    dma.channel[5].qwc=1u;
    assert(ps2_dma_fabric_step_bus(&fabric,&dma,&bus,&serviced)==1);
    for(i=0;i<16u;i++) assert(scratch[i]==0xC3u);
    /* Chain mode through the same fabric: CNT payload followed by END. */
    memset(ram,0,sizeof ram); dev.calls=0;
    write_tag(ram+64u,1u,PS2_DMA_TAG_CNT,0u);
    for(i=0;i<16u;i++)ram[80u+i]=(uint8_t)(0x40u+i);
    write_tag(ram+96u,0u,PS2_DMA_TAG_END,0u);
    dma.channel[5].chcr=PS2_DMA_CHCR_STR|PS2_DMA_CHCR_DIR|(UINT32_C(1)<<2);
    dma.channel[5].tadr=64u; dma.channel[5].qwc=0u;
    assert(ps2_dma_fabric_step_bus(&fabric,&dma,&bus,&serviced)==1);
    assert(serviced==5u && dev.calls==1u);
    assert(dev.last[0]==0x40u && dev.last[15]==0x4fu);
    assert(ps2_dma_fabric_take_completion(&fabric,5u)==0);
    assert(ps2_dma_fabric_step_bus(&fabric,&dma,&bus,&serviced)==1);
    assert((dma.channel[5].chcr&PS2_DMA_CHCR_STR)==0u);
    assert(ps2_dma_fabric_take_completion(&fabric,5u)==1);
    /* DPCR arbitration: priority 0 wins; equal priorities favor higher channel. */
    assert(ps2_dma_fabric_bind(&fabric,PS2_DMA_ENDPOINT_IPU_TO,transfer,&dev)==0);
    assert(ps2_dma_set_channel_priority(&dma,4u,1u,1)==0);
    assert(ps2_dma_set_channel_priority(&dma,5u,2u,1)==0);
    dma.channel[4].chcr=PS2_DMA_CHCR_STR|PS2_DMA_CHCR_DIR;
    dma.channel[4].madr=0u; dma.channel[4].qwc=1u;
    dma.channel[5].chcr=PS2_DMA_CHCR_STR|PS2_DMA_CHCR_DIR;
    dma.channel[5].madr=16u; dma.channel[5].qwc=1u;
    assert(ps2_dma_fabric_step_bus(&fabric,&dma,&bus,&serviced)==1);
    assert(serviced==4u);
    assert(ps2_dma_fabric_step_bus(&fabric,&dma,&bus,&serviced)==1);
    assert(serviced==5u);

    /* Same priority: the documented higher channel number wins arbitration. */
    dma.channel[4].chcr=PS2_DMA_CHCR_STR|PS2_DMA_CHCR_DIR;
    dma.channel[4].madr=0u; dma.channel[4].qwc=1u;
    dma.channel[5].chcr=PS2_DMA_CHCR_STR|PS2_DMA_CHCR_DIR;
    dma.channel[5].madr=16u; dma.channel[5].qwc=1u;
    assert(ps2_dma_set_channel_priority(&dma,4u,3u,1)==0);
    assert(ps2_dma_set_channel_priority(&dma,5u,3u,1)==0);
    assert(ps2_dma_fabric_step_bus(&fabric,&dma,&bus,&serviced)==1);
    assert(serviced==5u);
    assert(ps2_dma_fabric_step_bus(&fabric,&dma,&bus,&serviced)==1);
    assert(serviced==4u);
    puts("OK: DMA bus, chain mode and DPCR priority arbitration tests.");
    return 0;
}
