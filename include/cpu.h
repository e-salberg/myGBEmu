#pragma once

#include <common.h>
#include <stdbool.h>
#include <stdint.h>
#include <instructions.h>

typedef struct {
    uint8_t a;
    uint8_t f;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t h;
    uint8_t l;
    uint16_t pc;
    uint16_t sp;
} cpu_registers;

typedef struct {
    cpu_registers regs;
    uint16_t fetched_data;
    uint16_t memory_destination;
    bool destination_is_memory;
    uint8_t current_opcode;
    instruction *current_instruction;
    bool halted;
    bool stepping;
    bool master_interrupt_enabled;
} cpu_context;


void cpu_init();
bool cpu_step();
uint16_t cpu_read_reg(reg_type rt);
void cpu_set_reg(reg_type rt, uint16_t val);

typedef void (*IN_PROC)(cpu_context *);
IN_PROC inst_get_processor(instruction_type type);

#define CPU_FLAG_Z CHECK_BIT(ctx->regs.f, 7)
#define CPU_FLAG_C CHECK_BIT(ctx->regs.f, 4)