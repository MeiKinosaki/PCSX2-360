#include "dma_chain.h"
#include <stddef.h>
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
    tag->pce=(uint8_t)((lo>>26)&3u);
    tag->id=(uint8_t)((lo>>28)&7u);
    tag->irq=(uint8_t)((lo>>31)&1u);
    tag->address=(uint32_t)((lo>>32)&UINT64_C(0x7fffffff));
    tag->spr=(uint8_t)(lo>>63);
    tag->upper_data=hi;
    return 0;
}
