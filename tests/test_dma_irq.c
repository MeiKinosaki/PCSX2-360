#include "dma.h"
#include "ee_state.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
int main(void)
{
    ps2_dma_controller_t dma; ps2_ee_state_t ee;
    ps2_dma_reset(&dma); ps2_ee_reset(&ee);
    assert(ps2_dma_set_channel_priority(&dma,0u,3u,1)==0);
    assert((dma.dpcr[0]&0xfu)==0xbu);
    assert(ps2_dma_set_channel_priority(&dma,9u,5u,1)==0);
    assert((dma.dpcr[1]&0xfu)==0xdu);
    assert(ps2_dma_write_dicr(&dma,0u,(1u<<24)|(1u<<8))==0);
    assert(ps2_dma_complete_channel(&dma,0u,&ee)==0);
    assert((dma.dicr[0]&(1u<<16))!=0u);
    assert((dma.dicr[0]&(1u<<31))!=0u);
    assert((ee.cause&(1u<<10))!=0u);
    assert(ps2_dma_irq_pending(&dma,0u)==1);
    assert(ps2_dma_write_dicr(&dma,0u,1u<<16)==0);
    assert(ps2_dma_irq_pending(&dma,0u)==0);
    assert(ps2_dma_write_dicr(&dma,1u,(1u<<24)|(1u<<8))==0);
    assert(ps2_dma_complete_channel(&dma,9u,&ee)==0);
    assert((dma.dicr[1]&(1u<<18))!=0u);
    assert((ee.cause&(1u<<10))!=0u);
    assert(ps2_dma_write_dicr(&dma,2u,0u)==-1);
    assert(ps2_dma_set_channel_priority(&dma,10u,1u,1)==-1);
    puts("OK: experimental DPCR/DICR and EE CP0 IP2 interrupt tests.");
    return 0;
}
