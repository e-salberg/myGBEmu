#pragma once

#include <stdint.h>

uint8_t read_wram(uint16_t address);
void write_wram(uint16_t address, uint8_t value);

uint8_t read_hram(uint8_t address);
void write_hram(uint16_t address, uint8_t value);