#include "dma.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
typedef struct transfer_log {
    unsigned calls, channel;
    uint32_t address;
    int direction, fail;
} transfer_log_t;
static int record_transfer(void *opaque, unsigned channel, uint32_t address, int direction)
{
    transfer_log_t *log = (transfer_log_t *)opaque;
    ++log->calls; log->channel = channel; log->address = address;
    log->direction = direction; return log->fail;
}
int main(void)
{
    ps2_dma_controller_t dma;
    transfer_log_t log = {0u, 0u, 0u, 0, 0};
    uint32_t value = 0u;
    ps2_dma_reset(&dma);
    assert(ps2_dma_read_channel(&dma, 0u, PS2_DMA_REG_CHCR, &value) == 0 && value == 0u);
    assert(ps2_dma_write_channel(&dma, 2u, PS2_DMA_REG_MADR, UINT32_C(0x00102000)) == 0);
    assert(ps2_dma_write_channel(&dma, 2u, PS2_DMA_REG_QWC, 2u) == 0);
    assert(ps2_dma_write_channel(&dma, 2u, PS2_DMA_REG_CHCR,
                                 PS2_DMA_CHCR_STR | PS2_DMA_CHCR_DIR) == 0);
    assert(ps2_dma_step(&dma, 2u, record_transfer, &log) == 1);
    assert(log.calls == 1u && log.channel == 2u &&
           log.address == UINT32_C(0x00102000) && log.direction == 1);
    assert(dma.channel[2].madr == UINT32_C(0x00102010) && dma.channel[2].qwc == 1u);
    assert(ps2_dma_channel_started(&dma, 2u) == 1);
    assert(ps2_dma_step(&dma, 2u, record_transfer, &log) == 1);
    assert(dma.channel[2].madr == UINT32_C(0x00102020) && dma.channel[2].qwc == 0u);
    assert(ps2_dma_channel_started(&dma, 2u) == 0);
    dma.channel[3].chcr = PS2_DMA_CHCR_STR; dma.channel[3].madr = 0x1000u;
    dma.channel[3].qwc = 1u; log.fail = 1;
    assert(ps2_dma_step(&dma, 3u, record_transfer, &log) == -4);
    assert(dma.channel[3].madr == 0x1000u && dma.channel[3].qwc == 1u);
    assert(ps2_dma_channel_started(&dma, 3u) == 1);
    dma.channel[4].chcr = PS2_DMA_CHCR_STR | (1u << 2);
    dma.channel[4].madr = 0x2000u; dma.channel[4].qwc = 1u; log.fail = 0;
    assert(ps2_dma_step(&dma, 4u, record_transfer, &log) == -2);
    dma.channel[5].chcr = PS2_DMA_CHCR_STR; dma.channel[5].madr = 0x2001u;
    dma.channel[5].qwc = 1u;
    assert(ps2_dma_step(&dma, 5u, record_transfer, &log) == -3);
    assert(ps2_dma_write_channel(&dma, 10u, PS2_DMA_REG_CHCR, 0u) == -1);
    assert(ps2_dma_channel_started(&dma, 10u) == -1);
    puts("OK: DMA normal-mode single-QWC service tests.");
    return 0;
}
