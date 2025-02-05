#include <ram.h>

typedef struct {
    uint8_t wram[0x2000];
    uint8_t hram[0x80];
} ram_context;

static ram_context ctx;

uint8_t read_wram(uint16_t address)
{
    // remove offset from memory map
    address -= 0xC000;
    return ctx.wram[address];
}

void write_wram(uint16_t address, uint8_t value)
{
    // remove offset from memory map
    address -= 0xC000;
    ctx.wram[address] = value;
}

uint8_t read_hram(uint8_t address)
{
    // remove the offset from memory map
    address -= 0xFF80;
    return ctx.hram[address];
}

void write_hram(uint16_t address, uint8_t value)
{
    // remove offset from memory map
    address -= 0xFF80;
    ctx.hram[address] = value;
}