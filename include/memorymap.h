#pragma once

#include <common.h>
#include <stdint.h>

uint8_t read_address_bus(uint16_t address);
void write_address_bus(uint16_t address, uint8_t value);

uint16_t read16_address_bus(uint16_t address);
void write16_address_bus(uint16_t address, uint16_t value);
