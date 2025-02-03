#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t entry[4];
    uint8_t logo[0x30];
    char title[16]; // later cartridges reduce the size to 15 or 11
    // manufacturer code
    //cgb flag
    uint16_t new_licensee_code;
    uint8_t sgb_flag;
    uint8_t cartridge_type;
    uint8_t rom_size;
    uint8_t ram_size;
    uint8_t destination_code;
    uint8_t old_licensee_code;
    uint8_t mask_rom_version;
    uint8_t header_checksum;
    uint16_t global_checksum;
} cartridge_header;

bool load_cartridge(char *cartridge);

uint8_t read_cartridge(uint16_t address);
void write_cartridge(uint16_t address, uint8_t value);