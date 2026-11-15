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
    if (fabric == 0 || endpoint < 0 || endpoint >= PS2_DMA_ENDPOINT_COUNT) return -1;
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
        if (result < 0) return result;
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
        result=ps2_dma_step_bus(dma,ch,bus,slot->transfer,slot->opaque);
        if (result < 0) return result;
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
