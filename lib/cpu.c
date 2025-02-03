#include <stdio.h>
#include <stdlib.h>
#include <cpu.h>
#include <emu.h>
#include <instructions.h>
#include <memorymap.h>

cpu_context ctx = {0};

void cpu_init() 
{
    ctx.regs.pc = 0x100;
    ctx.regs.a = 0x01;
}

static void fetch_instruction()
{
    ctx.current_opcode = read_address_bus(ctx.regs.pc++);
    ctx.current_instruction = get_instruction_by_opcode(ctx.current_opcode);
}

static void fetch_data()
{   
    if (ctx.current_instruction == NULL)
    {
        return;
    }

    ctx.memory_destination = 0;
    ctx.destination_is_memory = false;
    switch(ctx.current_instruction->mode)
    {
        case AM_IMP:
            return;
        case AM_R:
            ctx.fetched_data = cpu_read_reg(ctx.current_instruction->reg_1);
            return;
        case AM_R_D8:
            ctx.fetched_data = read_address_bus(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;
            return;
        case AM_D16:
            ctx.fetched_data = read16_address_bus(ctx.regs.pc);
            emu_cycles(2);
            ctx.regs.pc += 2;
            return;
        default:
            printf("Unknown Addressing Mode! %d (%02X)\n", ctx.current_instruction->mode, ctx.current_opcode);
            exit(-7);
    }
}

static void execute() 
{
    IN_PROC proc = inst_get_processor(ctx.current_instruction->type);
    if (!proc) 
    {
        NO_IMPL
    }
    proc(&ctx);
}

bool cpu_step()
{
    if (!ctx.halted) 
    {
        uint16_t pc = ctx.regs.pc;
        fetch_instruction();
        fetch_data();

        printf("%04X: %-7s (%02X %02X %02X) A: %02X B: %02X C: %02X\n", 
            pc, get_instruction_name(ctx.current_instruction->type), ctx.current_opcode,
            read_address_bus(pc + 1), read_address_bus(pc + 2), ctx.regs.a, ctx.regs.b, ctx.regs.c);

        if (ctx.current_instruction == NULL)
        {
            printf("Unknown Instruction! %02X\n", ctx.current_opcode);
            exit(-7);
        }

        execute();
    }
    return true;
}