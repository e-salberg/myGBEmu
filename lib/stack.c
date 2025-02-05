#include <stack.h>
#include <cpu.h>
#include <memorymap.h>

void stack_push(uint8_t data)
{
    //cpu_get_regs()->sp--;
    write_address_bus(--cpu_get_regs()->sp, data);
}

void stack_push16(uint8_t data)
{
    stack_push((data >> 8) & 0xFF);
    stack_push(data & 0xFF);
}

uint8_t stack_pop()
{
    return read_address_bus(cpu_get_regs()->sp++);
}

uint16_t stack_pop16()
{
    uint16_t lo = stack_pop();
    uint16_t hi = stack_pop();
    return (hi << 8) | lo;
}