#include <cpu.h>
#include <emu.h>
#include <memorymap.h>
#include <stack.h>

// processes CPU instructions...

static bool check_condition(cpu_context *ctx)
{
    bool z = CPU_FLAG_Z;
    bool c = CPU_FLAG_C;

    switch(ctx->current_instruction->cond)
    {
        case CT_NONE:
            return true;
        case CT_C:
            return c;
        case CT_NC:
            return !c;
        case CT_Z:
            return z;
        case CT_NZ:
            return !z;
    }
    return false;
}

void cpu_set_flags(cpu_context *ctx, char z, char n, char h, char c)
{
    if (z != -1)
    {
        SET_BIT(ctx->regs.f, 7, z);
    }
    if (n != -1)
    {
        SET_BIT(ctx->regs.f, 6, n);
    }
    if (h != -1)
    {
        SET_BIT(ctx->regs.f, 5, h);
    }
    if (c != -1)
    {
        SET_BIT(ctx->regs.f, 4, h);
    }
}

static bool is_16_bit(reg_type rt)
{
    return rt >= RT_AF;
}

static void proc_none(cpu_context *ctx)
{
    printf("INVALID INSTRUCTION!\n");
    exit(-7);
}

static void proc_nop(cpu_context *ctx)
{
    // nop doesn't do anything
}

static void proc_di(cpu_context *ctx)
{
    ctx->master_interrupt_enabled = false;
}

static void proc_ld(cpu_context *ctx)
{   
    if (ctx->destination_is_memory)
    {
        // LD (BC), A 
        if (is_16_bit(ctx->current_instruction->reg_2))
        {
            emu_cycles(1);
            write16_address_bus(ctx->memory_destination, ctx->fetched_data);  
        } 
        else 
        {
            write_address_bus(ctx->memory_destination, ctx->fetched_data);
        }
        emu_cycles(1);
        return;
    }

    if (ctx->current_instruction->mode = AM_HL_SPR)
    {
        // LD HL, SP+e8
        /*
        This way looks more in line with the docs but not exactly sure if it works
        uint8_t carry_bit = (cpu_read_reg(ctx->current_instruction->reg_2) & 0xFF) + 
            (ctx->fetched_data & 0xFF);
        //for h flag docs say carry_bit[3] so do I shift over 3 times? i think 4 will match below
        uint8_t h = CHECK_BIT(carry_bit, 4); 3 or 4?
        // for c flag docs say carry_bit[7]
        uint8_t c = CHECK_BIT(carry_bit, 8); 7 or 8?
        */
        uint8_t h = (cpu_read_reg(ctx->current_instruction->reg_2) & 0xF) + 
            (ctx->fetched_data & 0xF) >= 0x10;
        uint8_t c = (cpu_read_reg(ctx->current_instruction->reg_2) & 0xFF) + 
            (ctx->fetched_data & 0xFF) >= 0x100;
        cpu_set_flags(ctx, 0, 0, h, c);
        cpu_set_reg(ctx->current_instruction->reg_1, 
            cpu_read_reg(ctx->current_instruction->reg_2) + (int8_t)ctx->fetched_data);
    }

    cpu_set_reg(ctx->current_instruction->reg_1, ctx->fetched_data);
}

static void proc_ldh(cpu_context *ctx)
{
    // LDH commands always use register A, either in pos 1 or pos 2
    if (ctx->current_instruction->reg_1 == RT_A)
    {
        // cpu_set_reg(ctx->current_instruction->reg_1, read_address_bus(0xFF00 | ctx->fetched_data));
        cpu_set_reg(ctx->current_instruction->reg_1, read_address_bus(ctx->fetched_data));
    }
    else 
    {
        write_address_bus(ctx->memory_destination, ctx->regs.a);
    }
    emu_cycles(1);
}

static void proc_xor(cpu_context *ctx)
{
    ctx->regs.a ^= ctx->fetched_data & 0xFF;
    cpu_set_flags(ctx, ctx->regs.a == 0, 0, 0, 0);
}

static void proc_jp(cpu_context *ctx)
{
    if (check_condition(ctx))
    {
        ctx->regs.pc = ctx->fetched_data;
        emu_cycles(1);
    }
}

static void proc_pop(cpu_context *ctx)
{
    uint16_t lo = stack_pop();
    emu_cycles(1);
    uint16_t hi = stack_pop();
    emu_cycles(1);

    uint16_t n = (hi << 8) | lo;
    cpu_set_reg(ctx->current_instruction->reg_1, n);

    // why is this the case? shouldn't POP AF completely replace the F register value?
    // check https://gekkio.fi/files/gb-docs/gbctr.pdf page 44
    /*if (ctx->current_instruction->reg_1 == RT_AF)
    {
        cpu_set_reg(ctx->current_instruction->reg_1, n & 0xFFF0);
    }*/
}

static void proc_push(cpu_context *ctx)
{
    //uint16_t hi = (cpu_read_reg(ctx->current_instruction->reg_1) >> 8) & 0xFF;
    uint8_t hi = (cpu_read_reg(ctx->current_instruction->reg_1) >> 8) & 0xFF;
    emu_cycles(1);
    stack_push(hi);

    //uint16_t lo = cpu_read_reg(ctx->current_instruction->reg_1) & 0xFF;
    uint8_t lo = cpu_read_reg(ctx->current_instruction->reg_1) & 0xFF;
    emu_cycles(1);
    stack_push(lo);

    emu_cycles(1);
}

static IN_PROC processors[] = {
    [IN_NONE] = proc_none,
    [IN_NOP] = proc_nop,
    [IN_LD] = proc_ld,
    [IN_LDH] = proc_ldh,
    [IN_JP] = proc_jp,
    [IN_DI] = proc_di,
    [IN_XOR] = proc_xor,
    [IN_POP] = proc_pop,
    [IN_PUSH] = proc_push
};

IN_PROC inst_get_processor(instruction_type type)
{
    return processors[type];
}