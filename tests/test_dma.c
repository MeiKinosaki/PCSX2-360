#include "dma.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct transfer_log { unsigned calls, channel; uint32_t address; int direction, fail; } transfer_log_t;
static int record_transfer(void *opaque, unsigned channel, uint32_t address, int direction)
{
    transfer_log_t *log = (transfer_log_t *)opaque;
    ++log->calls; log->channel = channel; log->address = address;
    log->direction = direction; return log->fail;
}
typedef struct device_log { unsigned calls; int fail; uint8_t received[16]; } device_log_t;
static int device_transfer(void *opaque, unsigned channel, uint8_t qword[16], int direction)
{
    device_log_t *log = (device_log_t *)opaque;
    (void)channel; ++log->calls;
    if (log->fail) return -1;
    if (direction) memcpy(log->received, qword, 16u);
    else memset(qword, 0xA5, 16u);
    return 0;
}
int main(void)
{
    ps2_dma_controller_t dma;
    transfer_log_t log = {0u, 0u, 0u, 0, 0};
    device_log_t device = {0u, 0, {0}};
    uint8_t ram[64];
    uint32_t value = 0u;
    unsigned i;
    ps2_dma_reset(&dma);
    assert(ps2_dma_read_channel(&dma, 0u, PS2_DMA_REG_CHCR, &value) == 0 && value == 0u);
    assert(ps2_dma_write_channel(&dma, 2u, PS2_DMA_REG_MADR, 0x20u) == 0);
    assert(ps2_dma_write_channel(&dma, 2u, PS2_DMA_REG_QWC, 2u) == 0);
    assert(ps2_dma_write_channel(&dma, 2u, PS2_DMA_REG_CHCR, PS2_DMA_CHCR_STR | PS2_DMA_CHCR_DIR) == 0);
    assert(ps2_dma_step(&dma, 2u, record_transfer, &log) == 1);
    assert(log.calls == 1u && log.channel == 2u && log.address == 0x20u && log.direction == 1);
    assert(dma.channel[2].madr == 0x30u && dma.channel[2].qwc == 1u);
    assert(ps2_dma_step(&dma, 2u, record_transfer, &log) == 1);
    assert(dma.channel[2].madr == 0x40u && dma.channel[2].qwc == 0u);
    assert(ps2_dma_channel_started(&dma, 2u) == 0);
    dma.channel[3].chcr = PS2_DMA_CHCR_STR; dma.channel[3].madr = 0x10u; dma.channel[3].qwc = 1u;
    log.fail = 1; assert(ps2_dma_step(&dma, 3u, record_transfer, &log) == -4);
    assert(dma.channel[3].madr == 0x10u && dma.channel[3].qwc == 1u);
    dma.channel[4].chcr = PS2_DMA_CHCR_STR | (1u << 2); dma.channel[4].madr = 0x20u; dma.channel[4].qwc = 1u;
    log.fail = 0; assert(ps2_dma_step(&dma, 4u, record_transfer, &log) == -2);
    dma.channel[5].chcr = PS2_DMA_CHCR_STR; dma.channel[5].madr = 0x21u; dma.channel[5].qwc = 1u;
    assert(ps2_dma_step(&dma, 5u, record_transfer, &log) == -3);
    assert(ps2_dma_write_channel(&dma, 10u, PS2_DMA_REG_CHCR, 0u) == -1);
    assert(ps2_dma_channel_started(&dma, 10u) == -1);

    for (i = 0; i < sizeof ram; ++i) ram[i] = (uint8_t)i;
    dma.channel[6].chcr = PS2_DMA_CHCR_STR | PS2_DMA_CHCR_DIR;
    dma.channel[6].madr = 16u; dma.channel[6].qwc = 1u;
    assert(ps2_dma_step_memory(&dma, 6u, ram, sizeof ram, device_transfer, &device) == 1);
    for (i = 0; i < 16u; ++i) assert(device.received[i] == (uint8_t)(16u + i));
    assert(dma.channel[6].madr == 32u && dma.channel[6].qwc == 0u);

    dma.channel[7].chcr = PS2_DMA_CHCR_STR; dma.channel[7].madr = 0u; dma.channel[7].qwc = 1u;
    assert(ps2_dma_step_memory(&dma, 7u, ram, sizeof ram, device_transfer, &device) == 1);
    for (i = 0; i < 16u; ++i) assert(ram[i] == 0xA5u);

    dma.channel[8].chcr = PS2_DMA_CHCR_STR; dma.channel[8].madr = 56u; dma.channel[8].qwc = 1u;
    assert(ps2_dma_step_memory(&dma, 8u, ram, sizeof ram, device_transfer, &device) == -5);
    assert(dma.channel[8].madr == 56u && dma.channel[8].qwc == 1u);
    device.fail = 1; dma.channel[9].chcr = PS2_DMA_CHCR_STR; dma.channel[9].madr = 0u; dma.channel[9].qwc = 1u;
    assert(ps2_dma_step_memory(&dma, 9u, ram, sizeof ram, device_transfer, &device) == -4);
    assert(dma.channel[9].madr == 0u && dma.channel[9].qwc == 1u);
    puts("OK: DMA normal-mode and EE RAM transfer tests.");
    return 0;
}
