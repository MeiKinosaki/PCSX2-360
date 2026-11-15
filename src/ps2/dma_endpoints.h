#ifndef PCSX2_360_DMA_ENDPOINTS_H
#define PCSX2_360_DMA_ENDPOINTS_H
#include "dma.h"
struct ps2_ee_bus;
typedef enum ps2_dma_endpoint {
    PS2_DMA_ENDPOINT_VIF0=0, PS2_DMA_ENDPOINT_VIF1, PS2_DMA_ENDPOINT_GIF,
    PS2_DMA_ENDPOINT_IPU_FROM, PS2_DMA_ENDPOINT_IPU_TO,
    PS2_DMA_ENDPOINT_SIF0, PS2_DMA_ENDPOINT_SIF1, PS2_DMA_ENDPOINT_SIF2,
    PS2_DMA_ENDPOINT_SPR_FROM, PS2_DMA_ENDPOINT_SPR_TO,
    PS2_DMA_ENDPOINT_COUNT
} ps2_dma_endpoint_t;
typedef struct ps2_dma_endpoint_slot {
    ps2_dma_device_qword_fn transfer;
    void *opaque;
} ps2_dma_endpoint_slot_t;
typedef struct ps2_dma_fabric {
    ps2_dma_endpoint_slot_t endpoint[PS2_DMA_ENDPOINT_COUNT];
    unsigned arbitration_cursor;
    uint16_t completion_pending;
} ps2_dma_fabric_t;
void ps2_dma_fabric_reset(ps2_dma_fabric_t *fabric);
int ps2_dma_fabric_bind(ps2_dma_fabric_t *fabric, ps2_dma_endpoint_t endpoint,
                        ps2_dma_device_qword_fn transfer, void *opaque);
int ps2_dma_fabric_step(ps2_dma_fabric_t *fabric, ps2_dma_controller_t *dma,
                        uint8_t *ee_ram, size_t ee_ram_size, unsigned *serviced_channel);
int ps2_dma_fabric_step_bus(ps2_dma_fabric_t *fabric, ps2_dma_controller_t *dma,
                            struct ps2_ee_bus *bus, unsigned *serviced_channel);
int ps2_dma_fabric_take_completion(ps2_dma_fabric_t *fabric, unsigned channel);
#endif
