#include "dma_endpoints.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct sink { unsigned calls; uint8_t data[16]; } sink_t;
static int sink_write(void *opaque,unsigned channel,uint8_t qword[16],int direction)
{
    sink_t *s=(sink_t*)opaque; (void)channel;
    if (!direction) memset(qword,0x5a,16u);
    else memcpy(s->data,qword,16u);
    ++s->calls; return 0;
}
int main(void)
{
    ps2_dma_controller_t dma; ps2_dma_fabric_t fabric; uint8_t ram[128]={0};
    sink_t vif={0,{0}},gif={0,{0}}; unsigned serviced=99u; unsigned i;
    ps2_dma_reset(&dma); ps2_dma_fabric_reset(&fabric);
    assert(ps2_dma_fabric_bind(&fabric,PS2_DMA_ENDPOINT_VIF0,sink_write,&vif)==0);
    assert(ps2_dma_fabric_bind(&fabric,PS2_DMA_ENDPOINT_GIF,sink_write,&gif)==0);
    for(i=0;i<16u;i++) ram[i]=(uint8_t)(i+1u);
    dma.channel[0].chcr=PS2_DMA_CHCR_STR|PS2_DMA_CHCR_DIR;
    dma.channel[0].madr=0u; dma.channel[0].qwc=1u;
    dma.channel[2].chcr=PS2_DMA_CHCR_STR; dma.channel[2].madr=16u; dma.channel[2].qwc=1u;
    assert(ps2_dma_fabric_step(&fabric,&dma,ram,sizeof ram,&serviced)==1 && serviced==0u);
    assert(vif.calls==1u && fabric.arbitration_cursor==1u);
    assert(ps2_dma_fabric_take_completion(&fabric,0u)==1);
    assert(ps2_dma_fabric_take_completion(&fabric,0u)==0);
    assert(ps2_dma_fabric_step(&fabric,&dma,ram,sizeof ram,&serviced)==1 && serviced==2u);
    assert(gif.calls==1u && ram[16]==0x5au);
    puts("OK: DMA endpoint routing, fair cursor and completion events.");
    return 0;
}
