#pragma once

#include <stdint.h>

void stack_push(uint8_t data);
void stack_push16(uint8_t data);

uint8_t stack_pop();
uint16_t stack_pop16();