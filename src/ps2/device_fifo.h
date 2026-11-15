#ifndef PCSX2_360_DEVICE_FIFO_H
#define PCSX2_360_DEVICE_FIFO_H
#include <stddef.h>
#include <stdint.h>
#define PS2_DEVICE_FIFO_CAPACITY 64u
#define PS2_DEVICE_FIFO_QWORD_BYTES 16u
typedef enum ps2_device_fifo_id {
    PS2_FIFO_VIF0=0, PS2_FIFO_VIF1, PS2_FIFO_GIF,
    PS2_FIFO_SIF0, PS2_FIFO_SIF1, PS2_FIFO_SIF2,
    PS2_FIFO_COUNT
} ps2_device_fifo_id_t;
typedef struct ps2_device_fifo {
    uint8_t qword[PS2_DEVICE_FIFO_CAPACITY][PS2_DEVICE_FIFO_QWORD_BYTES];
    unsigned read_index, write_index, count;
} ps2_device_fifo_t;
typedef struct ps2_device_fifos { ps2_device_fifo_t fifo[PS2_FIFO_COUNT]; } ps2_device_fifos_t;
typedef struct ps2_device_fifo_endpoint {
    ps2_device_fifos_t *fifos;
    ps2_device_fifo_id_t id;
} ps2_device_fifo_endpoint_t;
void ps2_device_fifos_reset(ps2_device_fifos_t *fifos);
int ps2_device_fifo_push(ps2_device_fifos_t *fifos, ps2_device_fifo_id_t id,
                         const uint8_t qword[PS2_DEVICE_FIFO_QWORD_BYTES]);
int ps2_device_fifo_pop(ps2_device_fifos_t *fifos, ps2_device_fifo_id_t id,
                        uint8_t qword[PS2_DEVICE_FIFO_QWORD_BYTES]);
int ps2_device_fifo_dma_transfer(void *opaque,unsigned channel,uint8_t qword[16],int direction);
size_t ps2_device_fifo_count(const ps2_device_fifos_t *fifos, ps2_device_fifo_id_t id);
#endif
