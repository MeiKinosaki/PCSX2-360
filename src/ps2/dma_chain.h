#ifndef PCSX2_360_DMA_CHAIN_H
#define PCSX2_360_DMA_CHAIN_H
#include <stdint.h>
typedef enum ps2_dma_tag_id {
    PS2_DMA_TAG_REFE=0, PS2_DMA_TAG_CNT=1, PS2_DMA_TAG_NEXT=2,
    PS2_DMA_TAG_REF=3, PS2_DMA_TAG_REFS=4, PS2_DMA_TAG_CALL=5,
    PS2_DMA_TAG_RET=6, PS2_DMA_TAG_END=7
} ps2_dma_tag_id_t;
typedef struct ps2_dma_tag {
    uint16_t qwc;
    uint8_t pce;
    uint8_t id;
    uint8_t irq;
    uint8_t spr;
    uint32_t address;
    uint64_t upper_data;
} ps2_dma_tag_t;
/* Decodes the 128-bit DMAC tag from little-endian bytes; no chain execution yet. */
int ps2_dma_tag_decode(const uint8_t bytes[16], ps2_dma_tag_t *tag);
#endif
