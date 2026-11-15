#include "ee_bus.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    uint8_t ram[PS2_EE_RAM_BYTES], scratch[PS2_EE_SCRATCHPAD_BYTES], q[16], out[16];
    ps2_ee_bus_t bus = {ram,sizeof ram,scratch,sizeof scratch};
    unsigned i;
    for(i=0;i<16u;i++) q[i]=(uint8_t)(0x40u+i);
    assert(ps2_ee_bus_write(&bus,0x100u,q,sizeof q)==0);
    memset(out,0,sizeof out);
    assert(ps2_ee_bus_read(&bus,0x100u,out,sizeof out)==0);
    assert(memcmp(q,out,sizeof q)==0);
    assert(ps2_ee_bus_write(&bus,PS2_EE_SCRATCHPAD_BASE+0x20u,q,sizeof q)==0);
    memset(out,0,sizeof out);
    assert(ps2_ee_bus_read(&bus,PS2_EE_SCRATCHPAD_BASE+0x20u,out,sizeof out)==0);
    assert(memcmp(q,out,sizeof q)==0);
    assert(ps2_ee_bus_read(&bus,0x02000000u,out,sizeof out)==-2);
    assert(ps2_ee_bus_read(&bus,PS2_EE_SCRATCHPAD_BASE+PS2_EE_SCRATCHPAD_BYTES-8u,out,16u)==-2);
    puts("OK: EE physical RAM and scratchpad bus tests.");
    return 0;
}
