#include "dma_chain.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    uint8_t raw[16]={0}; ps2_dma_tag_t tag;
    uint64_t low=UINT64_C(0x12345678);
    low|=UINT64_C(0x2a)<<0; low|=UINT64_C(2)<<26;
    low|=UINT64_C(5)<<28; low|=UINT64_C(1)<<31;
    low|=UINT64_C(0x0012345)<<32; low|=UINT64_C(1)<<63;
    for(unsigned i=0;i<8u;i++) raw[i]=(uint8_t)(low>>(i*8u));
    raw[8]=0xef; raw[9]=0xbe;
    assert(ps2_dma_tag_decode(raw,&tag)==0);
    assert(tag.qwc==0x2au && tag.pce==2u && tag.id==PS2_DMA_TAG_CALL);
    assert(tag.irq==1u && tag.spr==1u && tag.address==0x0012345u);
    assert((tag.upper_data&0xffffu)==0xbeefu);
    assert(ps2_dma_tag_decode(0, &tag)==-1);
    puts("OK: EE DMA tag wire-format decode tests.");
    return 0;
}
