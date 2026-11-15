#ifndef PCSX2_360_DMA_CHAIN_H
#define PCSX2_360_DMA_CHAIN_H
#include "dma.h"
#include "ee_bus.h"
#include <stdint.h>
typedef enum ps2_dma_tag_id {
    PS2_DMA_TAG_REFE=0, PS2_DMA_TAG_CNT=1, PS2_DMA_TAG_NEXT=2,
    PS2_DMA_TAG_REF=3, PS2_DMA_TAG_REFS=4, PS2_DMA_TAG_CALL=5,
    PS2_DMA_TAG_RET=6, PS2_DMA_TAG_END=7
} ps2_dma_tag_id_t;
typedef struct ps2_dma_tag {
    uint16_t qwc;
    uint8_t pce, id, irq, spr;
    uint32_t address;
    uint64_t upper_data;
} ps2_dma_tag_t;
#define PS2_DMA_CHAIN_STACK_DEPTH 2u
typedef struct ps2_dma_chain_state {
    uint32_t tag_address, data_address, next_tag_address;
    uint16_t remaining_qwc;
    uint8_t active, tag_loaded, finished, stack_depth;
    uint32_t return_stack[PS2_DMA_CHAIN_STACK_DEPTH];
    uint8_t last_tag_id, last_tag_irq;
} ps2_dma_chain_state_t;
int ps2_dma_tag_decode(const uint8_t bytes[16], ps2_dma_tag_t *tag);
void ps2_dma_chain_reset(ps2_dma_chain_state_t *state, uint32_t tadr);
int ps2_dma_chain_step(ps2_dma_chain_state_t *state, ps2_ee_bus_t *bus,
                       ps2_dma_device_qword_fn device, void *opaque,
                       unsigned channel, int direction, int tag_irq_enable);
#endif
