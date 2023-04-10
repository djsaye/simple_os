#include "pci.h"
#include "types.h"
#include "lib.h"
#include "outl.h"

/*
CONFIG ADDRESS REGISTER: 32 bit total
|  31  |  30-24 |  23-16   |    15-11    |      10-8     |     7 - 0     |
|Enable|Reserved|Bus Number|Device Number|Function Number|Register Offset|
Among them, register offset is word aligned so the last two bits are always 0
And enable bit is always 1
*/
uint32_t read_pci_conf(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    // params are shorts because they are 8 bits in the config register
    // calculating the config reg requires us to extend them to long and shift
    uint32_t bus_bits = ((uint32_t) bus) << 16;
    uint32_t device_bits = ((uint32_t) device) << 11;
    uint32_t func_bits = ((uint32_t) func) << 8;
    uint32_t enable_bits = 0x80000000;
    // ands with 0b11111100 to enforce the word alignment
    uint32_t offset_bits = ((uint32_t) offset) & 0xFC;
    uint32_t ret;

    uint32_t address = enable_bits + bus_bits + device_bits + func_bits + offset_bits;
    // Write out the address
    outl_asm(address, 0xCF8);
    // Read in the data
    // (offset & 2) * 8) = 0 will choose the first word of the 32-bit register
    ret = inl(0xCFC);
    return ret;
}
