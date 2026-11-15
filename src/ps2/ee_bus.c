#include "ee_bus.h"
#include <string.h>
static int resolve(ps2_ee_bus_t *bus, uint32_t address, size_t size,
                   uint8_t **base, size_t *offset)
{
    if (bus == 0 || base == 0 || offset == 0 || size == 0u) return -1;
    if (address < PS2_EE_RAM_BYTES && bus->ram != 0 &&
        (size_t)address <= bus->ram_size &&
        size <= bus->ram_size - (size_t)address) {
        *base = bus->ram; *offset = (size_t)address; return 0;
    }
    if (address >= PS2_EE_SCRATCHPAD_BASE && bus->scratchpad != 0) {
        uint32_t off = address - PS2_EE_SCRATCHPAD_BASE;
        if (off < PS2_EE_SCRATCHPAD_BYTES &&
            (size_t)off <= bus->scratchpad_size &&
            size <= bus->scratchpad_size - (size_t)off) {
            *base = bus->scratchpad; *offset = (size_t)off; return 0;
        }
    }
    return -2;
}
int ps2_ee_bus_read(ps2_ee_bus_t *bus, uint32_t address, void *out, size_t size)
{
    uint8_t *base; size_t offset;
    if (out == 0) return -1;
    if (resolve(bus,address,size,&base,&offset)!=0) return -2;
    memcpy(out,base+offset,size); return 0;
}
int ps2_ee_bus_write(ps2_ee_bus_t *bus, uint32_t address, const void *data, size_t size)
{
    uint8_t *base; size_t offset;
    if (data == 0) return -1;
    if (resolve(bus,address,size,&base,&offset)!=0) return -2;
    memcpy(base+offset,data,size); return 0;
}
