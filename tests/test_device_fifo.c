#include "device_fifo.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    ps2_device_fifos_t f; uint8_t in[16],out[16]; unsigned i;
    ps2_device_fifos_reset(&f);
    for(i=0;i<16u;i++)in[i]=(uint8_t)i;
    assert(ps2_device_fifo_pop(&f,PS2_FIFO_VIF0,out)==-2);
    assert(ps2_device_fifo_push(&f,PS2_FIFO_VIF0,in)==0);
    assert(ps2_device_fifo_count(&f,PS2_FIFO_VIF0)==1u);
    assert(ps2_device_fifo_count(&f,PS2_FIFO_GIF)==0u);
    memset(out,0,sizeof out);
    assert(ps2_device_fifo_pop(&f,PS2_FIFO_VIF0,out)==0);
    assert(memcmp(in,out,16u)==0);
    for(i=0;i<PS2_DEVICE_FIFO_CAPACITY;i++)assert(ps2_device_fifo_push(&f,PS2_FIFO_SIF0,in)==0);
    assert(ps2_device_fifo_push(&f,PS2_FIFO_SIF0,in)==-2);
    for(i=0;i<PS2_DEVICE_FIFO_CAPACITY;i++)assert(ps2_device_fifo_pop(&f,PS2_FIFO_SIF0,out)==0);
    assert(ps2_device_fifo_count(&f,PS2_FIFO_SIF0)==0u);
    puts("OK: VIF/GIF/SIF isolated bounded FIFO tests.");
    return 0;
}
