#ifndef PCSX2_360_DMA_H
#define PCSX2_360_DMA_H

#include <stdint.h>

#define PS2_DMA_CHANNEL_COUNT 10u
#define PS2_DMA_CHCR_STR (UINT32_C(1) << 8)

typedef struct ps2_dma_channel {
    uint32_t chcr;
    uint32_t madr;
    uint32_t qwc;
    uint32_t tadr;
} ps2_dma_channel_t;

typedef struct ps2_dma_controller {
    ps2_dma_channel_t channel[PS2_DMA_CHANNEL_COUNT];
    uint32_t dpcr[2];
    uint32_t dicr[2];
} ps2_dma_controller_t;

typedef enum ps2_dma_register {
    PS2_DMA_REG_CHCR = 0,
    PS2_DMA_REG_MADR,
    PS2_DMA_REG_QWC,
    PS2_DMA_REG_TADR
} ps2_dma_register_t;

void ps2_dma_reset(ps2_dma_controller_t *dma);
int ps2_dma_read_channel(const ps2_dma_controller_t *dma, unsigned channel,
                         ps2_dma_register_t reg, uint32_t *value);
int ps2_dma_write_channel(ps2_dma_controller_t *dma, unsigned channel,
                          ps2_dma_register_t reg, uint32_t value);
int ps2_dma_channel_started(const ps2_dma_controller_t *dma, unsigned channel);

#endif
