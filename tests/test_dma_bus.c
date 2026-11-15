#include "dma_endpoints.h"
#include "ee_bus.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct dev { unsigned calls; uint8_t last[16]; } dev_t;
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
    puts("OK: DMA endpoint fabric through EE physical bus.");
    return 0;
}
