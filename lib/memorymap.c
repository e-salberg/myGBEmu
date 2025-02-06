#include <memorymap.h>
#include <cartridge.h>
#include <ram.h>
#include <cpu.h>

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
// 0xFF80 - 0xFFFE : High RAM (HRAM) or Zero Page
// 0xFFFF - 0xFFFF : Interrupt ENable register

uint8_t read_address_bus(uint16_t address)
{
    if (address < 0x8000) 
    {
        // ROM Data
        return read_cartridge(address);
    }
    else if (address < 0xA000)
    {
        // Char/Map Data
        printf("UNSUPPORTED bus read(%04X)\n", address);
        NO_IMPL
    }
    else if (address < 0xC000)
    {
        //Cartridge RAM
        return read_cartridge(address);
    }
    else if (address < 0xE000)
    {
        // WRAM (Work RAM)
        return read_wram(address);
    }
    else if (address < 0xFE00)
    {
        // Reserved - Echo RAM
        return 0;
    }
    else if (address < 0xFEA0)
    {
        // OAM (Object Attribute Memory)
        printf("UNSUPPORTED bus read(%04X)\n", address);
        NO_IMPL
    }
    else if (address < 0xFF00)
    {
        // reserved - ususable
        return 0;
    }
    else if (address < 0xFF80)
    {
        // I/O Registers
        printf("UNSUPPORTED bus read(%04X)\n", address);
        NO_IMPL
    }
    else if (address < 0xFFFF)
    {
        // HRAM (High RAM)
        return read_hram(address);
    }
    else if (address == 0xFFFF)
    {
        // CPU Interrupt ENable register
        return cpu_get_ie_register();
    }
    printf("UNSUPPORTED bus read(%04X)\n", address);
    exit(-5);
}

void write_address_bus(uint16_t address, uint8_t value)
{
    if (address < 0x8000)
    {
        // ROM Data
        write_cartridge(address, value);
    }
    else if (address < 0xA000)
    {
        // Char/Map Data
        printf("UNSUPPORTED bus write(%04X)\n", address);
        NO_IMPL
    }
    else if (address < 0xC000)
    {
        //Cartridge RAM
        write_cartridge(address, value);
    }
    else if (address < 0xE000)
    {
        // WRAM (Working RAM)
        write_wram(address, value);
    }
    else if (address < 0xFE00)
    {
        // Reserved - Echo RAM
        return;
    }
    else if (address < 0xFEA0)
    {
        // OAM (Object Attribute Memory)
        printf("UNSUPPORTED bus write(%04X)\n", address);
        NO_IMPL
    }
    else if (address < 0xFF00)
    {
        // reserved - ususable
        return;
    }
    else if (address < 0xFF80)
    {
        // I/O Registers
        printf("UNSUPPORTED bus write(%04X)\n", address);
    }
    else if (address < 0xFFFF)
    {
        // HRAM (High RAM)
        write_hram(address, value);
    }
    else if (address == 0xFFFF)
    {
        // CPU Interrupt Enable register
        cpu_set_ie_register(value);
    }
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