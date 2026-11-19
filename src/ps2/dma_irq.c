#include "dma.h"
#include "ee_state.h"
#include <stddef.h>

#define DICR_MASTER_ENABLE (UINT32_C(1) << 23)
#define DICR_MASTER_FLAG   (UINT32_C(1) << 31)
#define DICR_BUS_ERROR     (UINT32_C(1) << 15)

static unsigned bank_for(unsigned channel) { return channel < 7u ? 0u : 1u; }
static unsigned local_for(unsigned channel) { return channel < 7u ? channel : channel - 7u; }
static uint32_t channel_mask(unsigned bank) { return bank == 0u ? UINT32_C(0x007f0000) : UINT32_C(0x003f0000); }
static uint32_t channel_flags(unsigned bank) { return bank == 0u ? UINT32_C(0x7f000000) : UINT32_C(0x3f000000); }

static void update_master_flag(ps2_dma_controller_t *dma)
{
    uint32_t flags = (dma->dicr[0] & channel_flags(0u)) |
                     (dma->dicr[1] & channel_flags(1u));
    int pending = (dma->dicr[0] & DICR_BUS_ERROR) != 0u ||
        (((dma->dicr[0] & DICR_MASTER_ENABLE) != 0u) && flags != 0u);
    if (pending) dma->dicr[0] |= DICR_MASTER_FLAG;
    else dma->dicr[0] &= ~DICR_MASTER_FLAG;
}

int ps2_dma_set_channel_priority(ps2_dma_controller_t *dma, unsigned channel,
                                 unsigned priority, int enabled)
{
    unsigned bank, shift;
    uint32_t mask;
    if (dma == 0 || channel >= PS2_DMA_CHANNEL_COUNT || priority > 7u) return -1;
    bank = bank_for(channel);
    shift = local_for(channel) * 4u;
    mask = UINT32_C(0xf) << shift;
    dma->dpcr[bank] = (dma->dpcr[bank] & ~mask) |
        ((uint32_t)priority << shift) | (enabled ? (UINT32_C(8) << shift) : 0u);
    return 0;
}

int ps2_dma_write_dicr(ps2_dma_controller_t *dma, unsigned bank, uint32_t value)
{
    uint32_t masks, flags;
    if (dma == 0 || bank > 1u) return -1;
    masks = channel_mask(bank);
    flags = channel_flags(bank);
    /* Completion flags are W1C; channel masks are ordinary RW bits.
       DICR master enable exists only in DICR0. */
    dma->dicr[bank] &= ~(value & flags);
    dma->dicr[bank] = (dma->dicr[bank] & ~(masks | (bank == 0u ? DICR_MASTER_ENABLE : 0u))) |
        (value & masks) | (bank == 0u ? (value & DICR_MASTER_ENABLE) : 0u);
    update_master_flag(dma);
    return 0;
}

int ps2_dma_irq_pending(const ps2_dma_controller_t *dma, unsigned bank)
{
    uint32_t flags;
    if (dma == 0 || bank > 1u) return 0;
    flags = dma->dicr[bank] & channel_flags(bank);
    if (bank == 0u)
        return (dma->dicr[0] & DICR_BUS_ERROR) != 0u ||
            (((dma->dicr[0] & DICR_MASTER_ENABLE) != 0u) && flags != 0u);
    return (dma->dicr[0] & DICR_MASTER_ENABLE) != 0u && flags != 0u;
}

void ps2_dma_sync_irq(const ps2_dma_controller_t *dma, struct ps2_ee_state *ee)
{
    if (dma == 0 || ee == 0) return;
    if (ps2_dma_irq_pending(dma, 0u) || ps2_dma_irq_pending(dma, 1u))
        ee->cause |= UINT32_C(1) << 10;
}

int ps2_dma_complete_channel(ps2_dma_controller_t *dma, unsigned channel,
                             struct ps2_ee_state *ee)
{
    unsigned bank, local;
    uint32_t mask, flag;
    if (dma == 0 || channel >= PS2_DMA_CHANNEL_COUNT) return -1;
    bank = bank_for(channel);
    local = local_for(channel);
    mask = UINT32_C(1) << (16u + local);
    flag = UINT32_C(1) << (24u + local);
    /* Per-channel mask gates the completion flag; global master enable
       gates propagation to the shared master interrupt flag. */
    if ((dma->dicr[bank] & mask) != 0u) dma->dicr[bank] |= flag;
    update_master_flag(dma);
    ps2_dma_sync_irq(dma, ee);
    return 0;
}
