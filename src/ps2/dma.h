#ifndef PCSX2_360_DMA_H
#define PCSX2_360_DMA_H
#include <stdint.h>
#define PS2_DMA_CHANNEL_COUNT 10u
#define PS2_DMA_CHCR_DIR (UINT32_C(1) << 0)
#define PS2_DMA_CHCR_MOD_MASK (UINT32_C(3) << 2)
#define PS2_DMA_CHCR_STR (UINT32_C(1) << 8)
#define PS2_DMA_QWORD_BYTES 16u
typedef struct ps2_dma_channel { uint32_t chcr, madr, qwc, tadr; } ps2_dma_channel_t;
typedef struct ps2_dma_controller {
    ps2_dma_channel_t channel[PS2_DMA_CHANNEL_COUNT];
    uint32_t dpcr[2], dicr[2];
} ps2_dma_controller_t;
typedef enum ps2_dma_register {
    PS2_DMA_REG_CHCR = 0, PS2_DMA_REG_MADR, PS2_DMA_REG_QWC, PS2_DMA_REG_TADR
} ps2_dma_register_t;
/* Services one 128-bit unit; device FIFO/bus behavior belongs to callback. */
typedef int (*ps2_dma_qword_transfer_fn)(void *opaque, unsigned channel,
                                         uint32_t memory_address, int direction);
void ps2_dma_reset(ps2_dma_controller_t *dma);
int ps2_dma_read_channel(const ps2_dma_controller_t *dma, unsigned channel,
                         ps2_dma_register_t reg, uint32_t *value);
int ps2_dma_write_channel(ps2_dma_controller_t *dma, unsigned channel,
                          ps2_dma_register_t reg, uint32_t value);
int ps2_dma_channel_started(const ps2_dma_controller_t *dma, unsigned channel);
/* One-QWC normal-mode service. 1=serviced, 0=idle, negative=error.
 * Chain mode, arbitration and device-specific endpoints are not implemented.
 */
int ps2_dma_step(ps2_dma_controller_t *dma, unsigned channel,
                 ps2_dma_qword_transfer_fn transfer, void *opaque);
#endif
