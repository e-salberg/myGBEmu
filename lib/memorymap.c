#include <memorymap.h>
#include <cartridge.h>

// **Memory Map Address Bus**
// 0x0000 - 0x3FFF : ROM Bank 0
// 0x4000 - 0x7FFF : ROM Bank 1 - Switchable
// 0x8000 - 0x97FF : CHR RAM
// 0x9800 - 0x9BFF : BG Map 1
// 0x9C00 - 0x9FFF : BG Map 2
// 0xA000 - 0xBFFF : Cartridge RAM
// 0xC000 - 0xCFFF : RAM Bank 0
// 0xD000 - 0xDFFF : RAM Bank 1-7 - switchable - Color only
// 0xE000 - 0xFDFF : Reserved - Echo RAM
// 0xFE00 - 0xFE9F : Object Attribute Memory
// 0xFEA0 - 0xFEFF : Reserved - Unusable
// 0xFF00 - 0xFF7F : I/O Registers
// 0xFF80 - 0xFFFE : Zero Page

uint8_t read_address_bus(uint16_t address)
{
    if (address < 0x8000) 
    {
        return read_cartridge(address);
    }
    NO_IMPL
}

void write_address_bus(uint16_t address, uint8_t value)
{
    if (address < 0x8000)
    {
        write_cartridge(address, value);
        return;
    }
    NO_IMPL
}

uint16_t read16_address_bus(uint16_t address)
{
    uint16_t lo = read_address_bus(address);
    uint16_t hi = read_address_bus(address + 1);

    return lo | (hi << 8);
}

void write16_address_bus(uint16_t address, uint16_t value)
{
    write_address_bus(address, value & 0xFF);
    write_address_bus(address + 1, (value >> 8) & 0xFF);
}   