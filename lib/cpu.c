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
        case AM_R_R:
            ctx.fetched_data = cpu_read_reg(ctx.current_instruction->reg_2);
            return;
        case AM_R_N8:
            ctx.fetched_data = read_address_bus(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;
            return;
        case AM_R_N16:
        case AM_N16: 
            uint16_t lo = read_address_bus(ctx.regs.pc);
            emu_cycles(1);

            uint16_t hi = read_address_bus(ctx.regs.pc + 1);
            emu_cycles(1);

            ctx.fetched_data = lo | (hi << 8);
            ctx.regs.pc += 2;
            return;
        case AM_MR_R:
            ctx.fetched_data = cpu_read_reg(ctx.current_instruction->reg_2);
            ctx.memory_destination = cpu_read_reg(ctx.current_instruction->reg_1);
            ctx.destination_is_memory = true;
            if (ctx.current_instruction->reg_1 == RT_C)
            {
                // special case for LDH [C], A
                ctx.memory_destination |= 0xFF00;
            }
            return;
        case AM_R_MR:
            uint16_t addr = cpu_read_reg(ctx.current_instruction->reg_2);
            if (ctx.current_instruction->reg_2 == RT_C)
            {
                // special case for LDH A, [C] 
                addr |= 0xFF00;
            }
            ctx.fetched_data = read_address_bus(addr);
            emu_cycles(1);
            return;
        case AM_R_HLI:
            ctx.fetched_data = read_address_bus(cpu_read_reg(ctx.current_instruction->reg_2));
            emu_cycles(1);
            cpu_set_reg(RT_HL, cpu_read_reg(RT_HL) + 1);
            return;
        case AM_R_HLD:
            ctx.fetched_data = read_address_bus(cpu_read_reg(ctx.current_instruction->reg_2));
            emu_cycles(1);
            cpu_set_reg(RT_HL, cpu_read_reg(RT_HL) - 1);
            return;
        case AM_HLI_R:
            ctx.fetched_data = cpu_read_reg(ctx.current_instruction->reg_2);
            ctx.memory_destination = ctx.current_instruction->reg_1;
            ctx.destination_is_memory = true;
            cpu_set_reg(RT_HL, cpu_read_reg(RT_HL) + 1);
            return;
        case AM_HLD_R:
            ctx.fetched_data = cpu_read_reg(ctx.current_instruction->reg_2);
            ctx.memory_destination = ctx.current_instruction->reg_1;
            ctx.destination_is_memory = true;
            cpu_set_reg(RT_HL, cpu_read_reg(RT_HL) - 1);
            return;
        case AM_N8:
        case AM_R_A8:
            //ctx.fetched_data = read_address_bus(ctx.regs.pc);
            ctx.fetched_data = read_address_bus(ctx.regs.pc) | 0xFF00;
            emu_cycles(1);
            ctx.regs.pc++;
            return;
        case AM_A8_R:
            ctx.fetched_data = cpu_read_reg(ctx.current_instruction->reg_2);
            ctx.memory_destination  = read_address_bus(ctx.regs.pc) | 0xFF00;
            ctx.destination_is_memory = true;
            emu_cycles(1);
            ctx.regs.pc++;
            return;
        case AM_HL_SPR:
            // special case for op:0xE8 -  LD HL, SP+e8
            ctx.fetched_data = read_address_bus(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;
            return;
        case AM_N16_R:
        case AM_A16_R:
            ctx.fetched_data = cpu_read_reg(ctx.current_instruction->reg_2);
            emu_cycles(2);
            ctx.regs.pc += 2;
            ctx.memory_destination = read16_address_bus(ctx.regs.pc);
            ctx.destination_is_memory = true;  
            return;
        case AM_R_A16:
            uint16_t address = read16_address_bus(ctx.regs.pc);
            emu_cycles(2);
            ctx.regs.pc += 2;
            ctx.fetched_data = read_address_bus(address);
            emu_cycles(1);
            return;
        case AM_MR_N8:
            ctx.fetched_data = read_address_bus(ctx.regs.pc);
            emu_cycles(1);
            ctx.regs.pc++;
            ctx.memory_destination = cpu_read_reg(ctx.current_instruction->reg_1);
            ctx.destination_is_memory = true;
            return;
        case AM_MR:
            ctx.memory_destination = cpu_read_reg(ctx.current_instruction->reg_1);
            ctx.destination_is_memory = true;
            ctx.fetched_data = read_address_bus(ctx.current_instruction->reg_1);
            emu_cycles(1);
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

        char flags[16];
        sprintf(flags, "%c%c%c%c", 
            ctx.regs.f & (1 << 7) ? 'Z' : '-',
            ctx.regs.f & (1 << 6) ? 'N' : '-',
            ctx.regs.f & (1 << 5) ? 'H' : '-',
            ctx.regs.f & (1 << 4) ? 'C' : '-' 
        ); 

        printf("%08lX - %04X: %-7s (%02X %02X %02X) A: %02X F: %s BC: %02X%02X DE: %02X%02X HL: %02X%02X\n", 
            emu_get_context()->ticks, 
            pc, get_instruction_name(ctx.current_instruction->type), ctx.current_opcode,
            read_address_bus(pc + 1), read_address_bus(pc + 2), ctx.regs.a, flags, ctx.regs.b, ctx.regs.c,
            ctx.regs.d, ctx.regs.e, ctx.regs.h, ctx.regs.l);

        if (ctx.current_instruction == NULL)
        {
            printf("Unknown Instruction! %02X\n", ctx.current_opcode);
            exit(-7);
        }

        execute();
    }
    return true;
}

cpu_registers *cpu_get_regs()
{
    return &ctx.regs;
}