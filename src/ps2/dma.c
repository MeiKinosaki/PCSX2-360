#include "dma.h"

#include <stddef.h>

void ps2_dma_reset(ps2_dma_controller_t *dma)
{
    if (dma == NULL) return;
    *dma = (ps2_dma_controller_t){0};
}

int ps2_dma_read_channel(const ps2_dma_controller_t *dma, unsigned channel,
                         ps2_dma_register_t reg, uint32_t *value)
{
    const ps2_dma_channel_t *ch;
    if (dma == NULL || value == NULL || channel >= PS2_DMA_CHANNEL_COUNT)
        return -1;
    ch = &dma->channel[channel];
    switch (reg) {
    case PS2_DMA_REG_CHCR: *value = ch->chcr; return 0;
    case PS2_DMA_REG_MADR: *value = ch->madr; return 0;
    case PS2_DMA_REG_QWC:  *value = ch->qwc; return 0;
    case PS2_DMA_REG_TADR: *value = ch->tadr; return 0;
    default: return -1;
    }
}

int ps2_dma_write_channel(ps2_dma_controller_t *dma, unsigned channel,
                          ps2_dma_register_t reg, uint32_t value)
{
    ps2_dma_channel_t *ch;
    if (dma == NULL || channel >= PS2_DMA_CHANNEL_COUNT) return -1;
    ch = &dma->channel[channel];
    switch (reg) {
    case PS2_DMA_REG_CHCR: ch->chcr = value; return 0;
    case PS2_DMA_REG_MADR: ch->madr = value; return 0;
    case PS2_DMA_REG_QWC:  ch->qwc = value; return 0;
    case PS2_DMA_REG_TADR: ch->tadr = value; return 0;
    default: return -1;
    }
}

int ps2_dma_channel_started(const ps2_dma_controller_t *dma, unsigned channel)
{
    if (dma == NULL || channel >= PS2_DMA_CHANNEL_COUNT) return -1;
    return (dma->channel[channel].chcr & PS2_DMA_CHCR_STR) != 0u;
}
