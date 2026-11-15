#include "dma_chain.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void write_tag(uint8_t *p,uint16_t qwc,unsigned id,uint32_t addr)
{
    uint64_t lo=(uint64_t)qwc|((uint64_t)id<<28)|((uint64_t)addr<<32);
    unsigned i; memset(p,0,16u);
    for(i=0;i<8u;i++)p[i]=(uint8_t)(lo>>(8u*i));
}
typedef struct sink { unsigned calls; uint8_t last[16]; } sink_t;
static int endpoint(void *opaque,unsigned channel,uint8_t q[16],int direction)
{
    sink_t *s=(sink_t*)opaque;(void)channel;++s->calls;
    if(direction)memcpy(s->last,q,16u);else memset(q,0x6d,16u);
    return 0;
}
int main(void)
{
    uint8_t ram[4096]={0},scratch[PS2_EE_SCRATCHPAD_BYTES];
    ps2_ee_bus_t bus={ram,sizeof ram,scratch,sizeof scratch};
    ps2_dma_chain_state_t s; sink_t sink={0,{0}}; unsigned i;
    write_tag(ram+0x100,1,PS2_DMA_TAG_CNT,0);
    for(i=0;i<16u;i++)ram[0x110u+i]=(uint8_t)(i+1u);
    write_tag(ram+0x120,0,PS2_DMA_TAG_END,0);
    ps2_dma_chain_reset(&s,0x100);
    assert(ps2_dma_chain_step(&s,&bus,endpoint,&sink,0,1)==1);
    assert(sink.calls==1u && sink.last[0]==1u && sink.last[15]==16u);
    assert(ps2_dma_chain_step(&s,&bus,endpoint,&sink,0,1)==2);
    assert(s.finished && !s.active);
    memset(ram,0,sizeof ram); sink.calls=0;
    write_tag(ram+0x100,1,PS2_DMA_TAG_CALL,0x200);
    for(i=0;i<16u;i++)ram[0x110u+i]=(uint8_t)(0x80u+i);
    write_tag(ram+0x120,0,PS2_DMA_TAG_END,0);
    write_tag(ram+0x200,0,PS2_DMA_TAG_RET,0);
    ps2_dma_chain_reset(&s,0x100);
    assert(ps2_dma_chain_step(&s,&bus,endpoint,&sink,1,1)==1);
    assert(s.stack_depth==1u);
    assert(ps2_dma_chain_step(&s,&bus,endpoint,&sink,1,1)==2);
    assert(s.finished && s.stack_depth==0u);
    /* Nested CALL/RET uses both return-stack entries in LIFO order. */
    memset(ram,0,sizeof ram);
    write_tag(ram+0x100,0,PS2_DMA_TAG_CALL,0x200);
    write_tag(ram+0x110,0,PS2_DMA_TAG_END,0);
    write_tag(ram+0x200,0,PS2_DMA_TAG_CALL,0x300);
    write_tag(ram+0x210,0,PS2_DMA_TAG_RET,0);
    write_tag(ram+0x300,0,PS2_DMA_TAG_RET,0);
    ps2_dma_chain_reset(&s,0x100);
    assert(ps2_dma_chain_step(&s,&bus,endpoint,&sink,1,1)==2);
    assert(s.finished && s.stack_depth==0u);

    /* A third nested CALL exceeds the two-entry hardware return stack. */
    memset(ram,0,sizeof ram);
    write_tag(ram+0x100,0,PS2_DMA_TAG_CALL,0x200);
    write_tag(ram+0x200,0,PS2_DMA_TAG_CALL,0x300);
    write_tag(ram+0x300,0,PS2_DMA_TAG_CALL,0x400);
    ps2_dma_chain_reset(&s,0x100);
    assert(ps2_dma_chain_step(&s,&bus,endpoint,&sink,1,1)==-5);

    memset(ram,0,sizeof ram); sink.calls=0;
    write_tag(ram+0x100,1,PS2_DMA_TAG_REF,0x300);
    for(i=0;i<16u;i++)ram[0x300u+i]=(uint8_t)(0x20u+i);
    write_tag(ram+0x110,0,PS2_DMA_TAG_END,0);
    ps2_dma_chain_reset(&s,0x100);
    assert(ps2_dma_chain_step(&s,&bus,endpoint,&sink,2,1)==1);
    assert(sink.last[0]==0x20u && sink.last[15]==0x2fu);
    assert(ps2_dma_chain_step(&s,&bus,endpoint,&sink,2,1)==2);
    memset(ram,0,sizeof ram); sink.calls=0;
    write_tag(ram+0x100,1,PS2_DMA_TAG_REFE,0x380);
    for(i=0;i<16u;i++)ram[0x380u+i]=(uint8_t)(0x50u+i);
    ps2_dma_chain_reset(&s,0x100);
    assert(ps2_dma_chain_step(&s,&bus,endpoint,&sink,3,1)==1);
    assert(sink.last[0]==0x50u && sink.last[15]==0x5fu);
    assert(ps2_dma_chain_step(&s,&bus,endpoint,&sink,3,1)==2);

    memset(ram,0,sizeof ram); sink.calls=0;
    write_tag(ram+0x100,0,PS2_DMA_TAG_NEXT,0x240);
    write_tag(ram+0x240,0,PS2_DMA_TAG_END,0);
    ps2_dma_chain_reset(&s,0x100);
    assert(ps2_dma_chain_step(&s,&bus,endpoint,&sink,4,1)==2);
    assert(s.finished);

    memset(ram,0,sizeof ram); sink.calls=0;
    write_tag(ram+0x100,1,PS2_DMA_TAG_REFS,0x3c0);
    for(i=0;i<16u;i++)ram[0x3c0u+i]=(uint8_t)(0x60u+i);
    write_tag(ram+0x110,0,PS2_DMA_TAG_END,0);
    ps2_dma_chain_reset(&s,0x100);
    assert(ps2_dma_chain_step(&s,&bus,endpoint,&sink,5,1)==1);
    assert(sink.last[0]==0x60u && sink.last[15]==0x6fu);
    assert(ps2_dma_chain_step(&s,&bus,endpoint,&sink,5,1)==2);

    memset(ram,0,sizeof ram); write_tag(ram+0x100,0,PS2_DMA_TAG_RET,0);
    ps2_dma_chain_reset(&s,0x100);
    assert(ps2_dma_chain_step(&s,&bus,endpoint,&sink,0,1)==-6);
    ps2_dma_chain_reset(&s,0x10000);
    assert(ps2_dma_chain_step(&s,&bus,endpoint,&sink,0,1)==-2);
    puts("OK: DMA chain CNT/END, CALL/RET, REF and error-path tests.");
    return 0;
}
