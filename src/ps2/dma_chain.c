#include "dma_chain.h"
#include <stddef.h>
#include <string.h>
static uint64_t read_le64(const uint8_t *p)
{
    uint64_t v=0; unsigned i;
    for(i=0;i<8u;i++) v|=(uint64_t)p[i]<<(i*8u);
    return v;
}
int ps2_dma_tag_decode(const uint8_t bytes[16], ps2_dma_tag_t *tag)
{
    uint64_t lo,hi;
    if(bytes==0 || tag==0) return -1;
    lo=read_le64(bytes); hi=read_le64(bytes+8);
    tag->qwc=(uint16_t)(lo&UINT64_C(0xffff));
    tag->pce=(uint8_t)((lo>>26)&3u); tag->id=(uint8_t)((lo>>28)&7u);
    tag->irq=(uint8_t)((lo>>31)&1u);
    tag->address=(uint32_t)((lo>>32)&UINT64_C(0x7fffffff));
    tag->spr=(uint8_t)(lo>>63); tag->upper_data=hi;
    return 0;
}
void ps2_dma_chain_reset(ps2_dma_chain_state_t *state, uint32_t tadr)
{
    if(state==0) return;
    memset(state,0,sizeof(*state)); state->tag_address=tadr; state->active=1u;
}
static int load_tag(ps2_dma_chain_state_t *s, ps2_ee_bus_t *bus)
{
    uint8_t raw[16]; ps2_dma_tag_t tag; uint32_t after;
    if(ps2_ee_bus_read(bus,s->tag_address,raw,sizeof raw)!=0) return -2;
    if(ps2_dma_tag_decode(raw,&tag)!=0) return -3;
    if(s->tag_address > UINT32_MAX-16u) return -4;
    after=s->tag_address+16u;
    s->last_tag_id=tag.id; s->last_tag_irq=tag.irq;
    s->remaining_qwc=tag.qwc;
    s->data_address=after;
    s->next_tag_address=after;
    switch(tag.id) {
    case PS2_DMA_TAG_REFE:
        s->data_address=tag.address; s->next_tag_address=0u; break;
    case PS2_DMA_TAG_CNT:
        if(tag.qwc > (UINT32_MAX-after)/16u) return -4;
        s->next_tag_address=after+(uint32_t)tag.qwc*16u; break;
    case PS2_DMA_TAG_NEXT:
        if(tag.qwc > (UINT32_MAX-after)/16u) return -4;
        s->next_tag_address=tag.address; break;
    case PS2_DMA_TAG_REF:
    case PS2_DMA_TAG_REFS:
        s->data_address=tag.address; s->next_tag_address=after; break;
    case PS2_DMA_TAG_CALL:
        if(tag.qwc > (UINT32_MAX-after)/16u) return -4;
        if(s->stack_depth>=PS2_DMA_CHAIN_STACK_DEPTH) return -5;
        s->return_stack[s->stack_depth++]=after+(uint32_t)tag.qwc*16u;
        s->next_tag_address=tag.address; break;
    case PS2_DMA_TAG_RET:
        if(s->stack_depth==0u) return -6;
        s->next_tag_address=s->return_stack[--s->stack_depth]; break;
    case PS2_DMA_TAG_END:
        if(tag.qwc > (UINT32_MAX-after)/16u) return -4;
        s->next_tag_address=0u; break;
    default: return -3;
    }
    s->tag_loaded=1u;
    return 0;
}
int ps2_dma_chain_step(ps2_dma_chain_state_t *s, ps2_ee_bus_t *bus,
                       ps2_dma_device_qword_fn device, void *opaque,
                       unsigned channel, int direction, int tag_irq_enable)
{
    uint8_t qword[16]; unsigned guard=0u; int rc;
    if(s==0 || bus==0 || device==0) return -1;
    if(s->finished || !s->active) return 2;
    while(s->remaining_qwc==0u) {
        if(++guard>PS2_DMA_CHAIN_STACK_DEPTH+4u) return -7;
        if(s->tag_loaded) {
            uint8_t id=s->last_tag_id;
            s->tag_loaded=0u;
            if((tag_irq_enable && s->last_tag_irq) || id==PS2_DMA_TAG_REFE || id==PS2_DMA_TAG_END) {
                s->finished=1u; s->active=0u; return 2;
            }
            s->tag_address=s->next_tag_address;
        }
        rc=load_tag(s,bus); if(rc!=0) return rc;
        if(s->last_tag_id==PS2_DMA_TAG_RET && s->remaining_qwc==0u) {
            s->tag_loaded=0u; s->tag_address=s->next_tag_address;
        }
    }
    if(s->data_address>UINT32_MAX-16u) return -4;
    if(direction) {
        if(ps2_ee_bus_read(bus,s->data_address,qword,sizeof qword)!=0) return -2;
    } else memset(qword,0,sizeof qword);
    if(device(opaque,channel,qword,direction)!=0) return -8;
    if(!direction && ps2_ee_bus_write(bus,s->data_address,qword,sizeof qword)!=0) return -2;
    s->data_address+=16u; --s->remaining_qwc;
    return 1;
}
