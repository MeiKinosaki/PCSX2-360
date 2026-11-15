#include "dma.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    ps2_dma_controller_t dma;
    uint32_t value = 0u;
    ps2_dma_reset(&dma);
    assert(ps2_dma_read_channel(&dma, 0u, PS2_DMA_REG_CHCR, &value) == 0);
    assert(value == 0u);
    assert(ps2_dma_write_channel(&dma, 2u, PS2_DMA_REG_MADR,
                                 UINT32_C(0x00102000)) == 0);
    assert(ps2_dma_write_channel(&dma, 2u, PS2_DMA_REG_QWC, 16u) == 0);
    assert(ps2_dma_write_channel(&dma, 2u, PS2_DMA_REG_TADR,
                                 UINT32_C(0x00103000)) == 0);
    assert(ps2_dma_write_channel(&dma, 2u, PS2_DMA_REG_CHCR,
                                 PS2_DMA_CHCR_STR) == 0);
    assert(ps2_dma_channel_started(&dma, 2u) == 1);
    assert(ps2_dma_read_channel(&dma, 2u, PS2_DMA_REG_MADR, &value) == 0);
    assert(value == UINT32_C(0x00102000));
    assert(ps2_dma_read_channel(&dma, 2u, PS2_DMA_REG_QWC, &value) == 0);
    assert(value == 16u);
    assert(ps2_dma_read_channel(&dma, 2u, PS2_DMA_REG_TADR, &value) == 0);
    assert(value == UINT32_C(0x00103000));
    assert(ps2_dma_write_channel(&dma, 10u, PS2_DMA_REG_CHCR, 0u) == -1);
    assert(ps2_dma_channel_started(&dma, 10u) == -1);
    puts("OK: DMA channel register scaffold.");
    return 0;
}
