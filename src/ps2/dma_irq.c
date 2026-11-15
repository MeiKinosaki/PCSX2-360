#include "dma.h"
#include "ee_state.h"
#include <stddef.h>
#define DICR_MASTER_ENABLE (UINT32_C(1)<<24)
#define DICR_MASTER_FLAG (UINT32_C(1)<<31)
static unsigned bank_for(unsigned channel){return channel<7u?0u:1u;}
static unsigned local_for(unsigned channel){return channel<7u?channel:channel-7u;}
int ps2_dma_set_channel_priority(ps2_dma_controller_t *dma,unsigned channel,
                                 unsigned priority,int enabled)
{
    unsigned bank,shift; uint32_t mask;
    if(dma==0 || channel>=PS2_DMA_CHANNEL_COUNT || priority>7u)return -1;
    bank=bank_for(channel); shift=local_for(channel)*4u; mask=UINT32_C(0xf)<<shift;
    dma->dpcr[bank]=(dma->dpcr[bank]&~mask)|
        ((uint32_t)priority<<shift)|(enabled?(UINT32_C(8)<<shift):0u);
    return 0;
}
int ps2_dma_write_dicr(ps2_dma_controller_t *dma,unsigned bank,uint32_t value)
{
    uint32_t flag_mask,old;
    if(dma==0 || bank>1u)return -1;
    flag_mask=bank==0u?UINT32_C(0x007f0000):UINT32_C(0x00070000);
    old=dma->dicr[bank];
    /* W1C completion flags; control bits are writable, master flag is derived. */
    old &= ~(value & flag_mask);
    old = (old & flag_mask) | (value & (DICR_MASTER_ENABLE | (bank==0u?UINT32_C(0x00007f00):UINT32_C(0x00000700))));
    dma->dicr[bank]=old;
    if(ps2_dma_irq_pending(dma,bank)) dma->dicr[bank]|=DICR_MASTER_FLAG;
    else dma->dicr[bank]&=~DICR_MASTER_FLAG;
    return 0;
}
int ps2_dma_irq_pending(const ps2_dma_controller_t *dma,unsigned bank)
{
    uint32_t flags,masks;
    if(dma==0 || bank>1u)return 0;
    flags=dma->dicr[bank] & (bank==0u?UINT32_C(0x007f0000):UINT32_C(0x00070000));
    masks=(dma->dicr[bank]>>8)&(bank==0u?UINT32_C(0x7f):UINT32_C(0x7));
    return (dma->dicr[bank]&DICR_MASTER_ENABLE)!=0u && (flags&masks)!=0u;
}
int ps2_dma_complete_channel(ps2_dma_controller_t *dma,unsigned channel,
                             struct ps2_ee_state *ee)
{
    unsigned bank,local; uint32_t flag;
    if(dma==0 || channel>=PS2_DMA_CHANNEL_COUNT)return -1;
    bank=bank_for(channel); local=local_for(channel); flag=UINT32_C(1)<<(16u+local);
    dma->dicr[bank]|=flag;
    if(ps2_dma_irq_pending(dma,bank)) {
        if(ee!=0) ee->cause |= UINT32_C(1)<<10; /* EE external interrupt line IP2. */
        dma->dicr[bank]|=DICR_MASTER_FLAG;
    } else dma->dicr[bank]&=~DICR_MASTER_FLAG;
    return 0;
}
