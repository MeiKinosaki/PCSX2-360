#include "dma_endpoints.h"
#include <stddef.h>
void ps2_dma_fabric_reset(ps2_dma_fabric_t *fabric)
{
    if (fabric == 0) return;
    *fabric = (ps2_dma_fabric_t){0};
}
int ps2_dma_fabric_bind(ps2_dma_fabric_t *fabric, ps2_dma_endpoint_t endpoint,
                        ps2_dma_device_qword_fn transfer, void *opaque)
{
    if (fabric == 0 || (unsigned)endpoint >= (unsigned)PS2_DMA_ENDPOINT_COUNT) return -1;
    fabric->endpoint[endpoint].transfer = transfer;
    fabric->endpoint[endpoint].opaque = opaque;
    return 0;
}
int ps2_dma_fabric_step(ps2_dma_fabric_t *fabric, ps2_dma_controller_t *dma,
                        uint8_t *ee_ram, size_t ee_ram_size, unsigned *serviced_channel)
{
    unsigned n;
    if (fabric == 0 || dma == 0 || ee_ram == 0) return -1;
    for (n=0; n<PS2_DMA_CHANNEL_COUNT; ++n) {
        unsigned ch=(fabric->arbitration_cursor+n)%PS2_DMA_CHANNEL_COUNT;
        ps2_dma_endpoint_slot_t *slot=&fabric->endpoint[ch];
        int was_started=ps2_dma_channel_started(dma,ch);
        int result;
        if (was_started != 1 || slot->transfer == 0) continue;
        result=ps2_dma_step_memory(dma,ch,ee_ram,ee_ram_size,slot->transfer,slot->opaque);
        if (result < 0) { dma->channel[ch].chcr&=~PS2_DMA_CHCR_STR; fabric->chain_initialized&=(uint16_t)~(uint16_t)(1u<<ch); return result; }
        if (result == 1) {
            if (dma->channel[ch].qwc == 0u)
                fabric->completion_pending |= (uint16_t)(1u<<ch);
            fabric->arbitration_cursor=(ch+1u)%PS2_DMA_CHANNEL_COUNT;
            if (serviced_channel != 0) *serviced_channel=ch;
            return 1;
        }
    }
    return 0;
}
int ps2_dma_fabric_take_completion(ps2_dma_fabric_t *fabric, unsigned channel)
{
    uint16_t bit;
    int pending;
    if (fabric == 0 || channel >= PS2_DMA_CHANNEL_COUNT) return -1;
    bit=(uint16_t)(1u<<channel);
    pending=(fabric->completion_pending & bit)!=0u;
    fabric->completion_pending &= (uint16_t)~bit;
    return pending;
}

int ps2_dma_fabric_step_bus(ps2_dma_fabric_t *fabric, ps2_dma_controller_t *dma,
                            struct ps2_ee_bus *bus, unsigned *serviced_channel)
{
    unsigned n;
    if (fabric == 0 || dma == 0 || bus == 0) return -1;
    for (n=0; n<PS2_DMA_CHANNEL_COUNT; ++n) {
        unsigned ch=(fabric->arbitration_cursor+n)%PS2_DMA_CHANNEL_COUNT;
        ps2_dma_endpoint_slot_t *slot=&fabric->endpoint[ch];
        int result;
        if (ps2_dma_channel_started(dma,ch) != 1 || slot->transfer == 0) continue;
        { unsigned bank=ch<7u?0u:1u; unsigned local=ch<7u?ch:ch-7u;
          if ((dma->dpcr[bank] & (UINT32_C(8)<<(local*4u)))==0u) continue; }
        if ((dma->channel[ch].chcr & PS2_DMA_CHCR_MOD_MASK)==(UINT32_C(1)<<2)) {
            uint16_t bit=(uint16_t)(1u<<ch);
            if ((fabric->chain_initialized & bit)==0u) {
                ps2_dma_chain_reset(&fabric->chain[ch],dma->channel[ch].tadr);
                fabric->chain_initialized|=bit;
            }
            result=ps2_dma_chain_step(&fabric->chain[ch],bus,slot->transfer,
                                      slot->opaque,ch,
                                      (dma->channel[ch].chcr&PS2_DMA_CHCR_DIR)!=0u,
                                      (dma->channel[ch].chcr&PS2_DMA_CHCR_TIE)!=0u);
            if(result==2) {
                dma->channel[ch].chcr&=~PS2_DMA_CHCR_STR;
                fabric->chain_initialized&=(uint16_t)~bit;
                fabric->completion_pending|=bit;
                result=1;
            }
            dma->channel[ch].tadr=fabric->chain[ch].tag_address;
        } else {
            result=ps2_dma_step_bus(dma,ch,bus,slot->transfer,slot->opaque);
            if(result==1 && dma->channel[ch].qwc==0u)
                fabric->completion_pending|=(uint16_t)(1u<<ch);
        }
        if (result < 0) return result;
        if (result == 1) {
            fabric->arbitration_cursor=(ch+1u)%PS2_DMA_CHANNEL_COUNT;
            if (serviced_channel != 0) *serviced_channel=ch;
            return 1;
        }
    }
    return 0;
}
