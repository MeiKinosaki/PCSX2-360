#ifndef PCSX2_360_EE_BUS_H
#define PCSX2_360_EE_BUS_H
#include <stddef.h>
#include <stdint.h>
#define PS2_EE_RAM_BYTES (32u * 1024u * 1024u)
#define PS2_EE_SCRATCHPAD_BASE UINT32_C(0x70000000)
#define PS2_EE_SCRATCHPAD_BYTES (16u * 1024u)
typedef struct ps2_ee_bus {
    uint8_t *ram;
    size_t ram_size;
    uint8_t *scratchpad;
    size_t scratchpad_size;
} ps2_ee_bus_t;
/* Physical EE bus access. Main RAM and scratchpad are distinct regions;
 * unmapped/device addresses are rejected rather than aliased into RAM.
 */
int ps2_ee_bus_read(ps2_ee_bus_t *bus, uint32_t address, void *out, size_t size);
int ps2_ee_bus_write(ps2_ee_bus_t *bus, uint32_t address, const void *data, size_t size);
#endif
