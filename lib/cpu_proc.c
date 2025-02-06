#include <cpu.h>
#include <emu.h>
#include <memorymap.h>
#include <stack.h>

// processes CPU instructions...

reg_type rt_lookup[] = {
    RT_B,
    RT_C,
    RT_D,
    RT_E,
    RT_H,
    RT_L,
    RT_HL,
    RT_A
};

reg_type decode_reg(uint8_t reg)
{
    if (reg > 0b111)
    {
        return RT_NONE;
    }
    return rt_lookup[reg];
}

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

static void goto_addr(cpu_context *ctx, uint16_t addr, bool pushpc)
{
    if (!check_condition(ctx))
    {
        return;
    }

    if (pushpc)
    {
        emu_cycles(2);
        stack_push16(ctx->regs.pc);
    }
    ctx->regs.pc = addr;
    emu_cycles(1);
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

static void proc_jp(cpu_context *ctx)
{
    goto_addr(ctx, ctx->fetched_data, false);
}

static void proc_jr(cpu_context *ctx)
{
    int8_t rel = (int8_t)(ctx->fetched_data & 0xFF);
    uint16_t addr = ctx->regs.pc + rel;
    goto_addr(ctx, addr, false);
}

static void proc_call(cpu_context *ctx)
{
    goto_addr(ctx, ctx->fetched_data, true);
}

static void proc_rst(cpu_context *ctx)
{
    goto_addr(ctx, ctx->current_instruction->param, true);
}

static void proc_ret(cpu_context *ctx)
{
    if (ctx->current_instruction->cond != CT_NONE)
    {
        // see page 121 of https://gekkio.fi/files/gb-docs/gbctr.pdf
        emu_cycles(1);
    }

    if (check_condition(ctx))
    {
        // 2 stack_pop instead of 1 stack_pop16 for cycle accuracy
        uint16_t lo = stack_pop();
        emu_cycles(1);
        uint16_t hi = stack_pop();
        emu_cycles(1);
        uint16_t n = (hi << 8) | lo;
        ctx->regs.pc = n;
        emu_cycles(1);
    }
}

static void proc_reti(cpu_context *ctx)
{
    ctx->master_interrupt_enabled = true;
    proc_ret(ctx);
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
        return;
    }

    cpu_set_reg(ctx->current_instruction->reg_1, ctx->fetched_data);
}

static void proc_ldh(cpu_context *ctx)
{
    // LDH commands always use register A, either in pos 1 or pos 2
    if (ctx->current_instruction->reg_1 == RT_A)
    {
        //cpu_set_reg(ctx->current_instruction->reg_1, read_address_bus(0xFF00 | ctx->fetched_data));
        cpu_set_reg(ctx->current_instruction->reg_1, read_address_bus(ctx->fetched_data));
    }
    else 
    {
        write_address_bus(ctx->memory_destination, ctx->regs.a);
    }
    emu_cycles(1);
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

static void proc_add(cpu_context *ctx)
{
    uint32_t val = cpu_read_reg(ctx->current_instruction->reg_1) + ctx->fetched_data;

    bool is_16bit = is_16_bit(ctx->current_instruction->reg_1);

    if (is_16bit)
    {
        emu_cycles(1);
    }

    // special case Add to stack point (relative) opcode = 0xE8: ADD SP, e8
    if (ctx->current_instruction->reg_1 == RT_SP)
    {
        val = cpu_read_reg(ctx->current_instruction->reg_1) + (int8_t)ctx->fetched_data;
    }


    int z =(val & 0xFF) == 0;
    int h = (cpu_read_reg(ctx->current_instruction->reg_1) & 0xF) + (ctx->fetched_data & 0xF) >= 0x10;
    int c = (int)(cpu_read_reg(ctx->current_instruction->reg_1) & 0xFF) + (int)(ctx->fetched_data & 0xFF) >= 0x100;

    if (is_16bit)
    {
        z = -1;
        h = (cpu_read_reg(ctx->current_instruction->reg_1) & 0xFFF) + (ctx->fetched_data & 0xFFF) >= 0x1000;
        uint32_t n = ((uint32_t)cpu_read_reg(ctx->current_instruction->reg_1)) + ((uint32_t)ctx->fetched_data);
        c = n >= 0x10000;
    }

    if (ctx->current_instruction->reg_1 == RT_SP)
    {
        z = 0;
        int h = (cpu_read_reg(ctx->current_instruction->reg_1) & 0xF) + (ctx->fetched_data & 0xF) >= 0x10;
        int c = (int)(cpu_read_reg(ctx->current_instruction->reg_1) & 0xFF) + (int)(ctx->fetched_data & 0xFF) >= 0x100;
    }
    cpu_set_reg(ctx->current_instruction->reg_1, val & 0xFFFF);
    cpu_set_flags(ctx, z, 0, h, c);
}

static void proc_adc(cpu_context *ctx)
{
    uint16_t u = ctx->fetched_data;
    uint16_t a = ctx->regs.a;
    uint16_t c = CPU_FLAG_C;

    ctx->regs.a = (a + u + c) & 0xFF;
    cpu_set_flags(ctx, ctx->regs.a == 0, 0, (a & 0xF) + (u & 0xF) + c > 0xF, a + u + c > 0xFF);
}

static void proc_sub(cpu_context *ctx)
{
    uint16_t val = cpu_read_reg(ctx->current_instruction->reg_1) - ctx->fetched_data;
    int z = val == 0;
    int h = ((int)cpu_read_reg(ctx->current_instruction->reg_1) & 0xF) - ((int)ctx->fetched_data & 0xF) < 0;
    int c = ((int)cpu_read_reg(ctx->current_instruction->reg_1)) - ((int)ctx->fetched_data) < 0;
    cpu_set_reg(ctx->current_instruction->reg_1, val);
    cpu_set_flags(ctx, z, 1, h, c);
}

static void proc_sbc(cpu_context *ctx)
{
    uint8_t val = ctx->fetched_data + CPU_FLAG_C;
    int z = cpu_read_reg(ctx->current_instruction->reg_1) - val == 0;
    int h = ((int)cpu_read_reg(ctx->current_instruction->reg_1) & 0xF) - ((int)ctx->fetched_data & 0xF) - ((int)CPU_FLAG_C) < 0;
    int c = ((int)cpu_read_reg(ctx->current_instruction->reg_1)) - ((int)ctx->fetched_data) - ((int)CPU_FLAG_C) < 0;
    cpu_set_reg(ctx->current_instruction->reg_1, cpu_read_reg(ctx->current_instruction->reg_1) - val);
    cpu_set_flags(ctx, z, 1, h, c);
}


static void proc_inc(cpu_context *ctx)
{
    uint16_t val = cpu_read_reg(ctx->current_instruction->reg_1) + 1;
    
    if (is_16_bit(ctx->current_instruction->reg_1))
    {
        emu_cycles(1);
    }

    // HL is the only reg that has an instruction with AM_MR
    if (ctx->current_instruction->reg_1 == RT_HL && ctx->current_instruction->mode == AM_MR)
    {
        val = read_address_bus(cpu_read_reg(RT_HL)) + 1;
        val &= 0xFF; // is this needed?
        write_address_bus(cpu_read_reg(RT_HL), val);
    }
    else
    {
        // why do we need to reread the value after setting it?
        cpu_set_reg(ctx->current_instruction->reg_1, val);
        val = cpu_read_reg(ctx->current_instruction->reg_1);
    }

    // INC op codes that end in x3 do not set the flags
    if ((ctx->current_opcode & 0x03) == 0x03)
    {
        return;
    }

    // for h why not CHECK_BIT(val, 5)?
    // why (val & 0x0F) == 0 instead of == 0x0F?
    cpu_set_flags(ctx, val == 0, 0, (val & 0x0F) == 0, -1);
}

static void proc_dec(cpu_context *ctx)
{
    uint16_t val = cpu_read_reg(ctx->current_instruction->reg_1) - 1;
    
    if (is_16_bit(ctx->current_instruction->reg_1))
    {
        emu_cycles(1);
    }

    // HL is the only reg that has an instruction with AM_MR
    if (ctx->current_instruction->reg_1 == RT_HL && ctx->current_instruction->mode == AM_MR)
    {
        val = read_address_bus(cpu_read_reg(RT_HL)) - 1;
        write_address_bus(cpu_read_reg(RT_HL), val);
    }
    else
    {
        // why do we need to reread the value after setting it?
        cpu_set_reg(ctx->current_instruction->reg_1, val);
        val = cpu_read_reg(ctx->current_instruction->reg_1);
    }

    // INC op codes that end in x3 do not set the flags
    if ((ctx->current_opcode & 0x0B) == 0x0B)
    {
        return;
    }

    // for h why not CHECK_BIT(val, 5)?
    cpu_set_flags(ctx, val == 0, 1, (val & 0x0F) == 0x0F, -1);
}

static void proc_and(cpu_context *ctx)
{
    ctx->regs.a &= ctx->fetched_data;
    cpu_set_flags(ctx, ctx->regs.a == 0, 0, 1, 0);
}

static void proc_or(cpu_context *ctx)
{
    ctx->regs.a |= ctx->fetched_data & 0xFF;
    cpu_set_flags(ctx, ctx->regs.a == 0, 0, 0, 0);
}

static void proc_xor(cpu_context *ctx)
{
    ctx->regs.a ^= ctx->fetched_data & 0xFF;
    cpu_set_flags(ctx, ctx->regs.a == 0, 0, 0, 0);
}

// this is basically the sam eas SUB r but does not update register A
static void proc_cp(cpu_context *ctx)
{
    int val = (int)ctx->regs.a - ctx->fetched_data;
    cpu_set_flags(ctx, val == 0, 1, ((int)ctx->regs.a & 0x0F) - ((int)ctx->fetched_data & 0x0F) < 0, val < 0);
}

static void proc_cb(cpu_context *ctx)
{
    uint8_t op = ctx->fetched_data;
    reg_type reg = decode_reg(op & 0b111);
    uint8_t bit = (op >> 3) & 0b111;
    uint8_t bit_op = (op >> 6) & 0b111;
    uint8_t reg_val = cpu_read_reg8(reg);

    emu_cycles(1);

    if (reg == RT_HL)
    {
        emu_cycles(2);
    }

    switch(bit_op) 
    {
        case 1: 
            // BIT
            cpu_set_flags(ctx, !(reg_val & (1 << bit)), 0, 1, -1);
            return;
        case 2: 
            //RST
            reg_val &= ~(1 << bit);
            cpu_set_reg8(reg, reg_val);
            return;
        case 3: 
            //SET
            reg_val |= ~(1 << bit);
            cpu_set_reg8(reg, reg_val);
            return;
    }

    bool flagC = CPU_FLAG_C;
    switch(bit) 
    {
        case 0: {
            //RLC - Rotate Left old bit 7 to carry flag
            bool setC = false;
            uint8_t result = (reg_val << 1) & 0xFF;

            // if bit 7 is not set on reg_val
            if ((reg_val & (1 << 7)) != 0)
            {
                result |= 1;
                setC = true;
            }
            cpu_set_reg8(reg, result);
            cpu_set_flags(ctx, result == 0, 0, 0, setC);
        } return;

        case 1: {
            //RRC - Rotate Right old bit 0 to carry flag
            uint8_t old = reg_val;
            reg_val >>= 1;
            reg_val |= (old << 7);
            cpu_set_reg8(reg, reg_val);
            cpu_set_flags(ctx, !reg_val, 0, 0, old & 1);

        } return;

        case 2: {
            //RL - Rotate Left
            uint8_t old = reg_val;
            reg_val <<= 1;
            reg_val |= flagC;
            cpu_set_reg8(reg, reg_val);
            // why !! is needed? old & 0x80 can be any value if set and !! makes it 1?
            cpu_set_flags(ctx, !reg_val, 0, 0, !!(old & 0x80));
        } return;

        case 3: {
            //RR - Rotate Right
            uint8_t old = reg_val;
            reg_val >>= 1;
            reg_val |= (flagC << 7);
            cpu_set_reg8(reg, reg_val);
            cpu_set_flags(ctx, !reg_val, 0, 0, old & 1);
        } return;

        case 4: {
            //SLA - Shift Left And carry
            uint8_t old = reg_val;
            reg_val <<= 1;
            cpu_set_reg8(reg, reg_val);
            cpu_set_flags(ctx, !reg_val, 0, 0, !!(old & 0x80));
        } return;

        case 5: {
            //SRA - Shift Right And carry
            uint8_t u = (int8_t)reg_val >> 1;
            cpu_set_reg8(reg, u);
            cpu_set_flags(ctx, !u, 0, 0, reg_val & 1);
        } return;

        case 6: {
            //SWAP - swap high and low nibbles
            reg_val = ((reg_val & 0xF0) >> 4) | ((reg_val & 0xF) << 4);
            cpu_set_reg8(reg, reg_val);
            cpu_set_flags(ctx, reg_val == 0, 0, 0, 0);
        } return;

        case 7: {
            //SRL
            uint8_t u = reg_val >> 1;
            cpu_set_reg8(reg, u);
            cpu_set_flags(ctx, !u, 0, 0, reg_val & 1);
        } return;
    }

    fprintf(stderr, "ERROR: INVALID CB: %02X", op);
    NO_IMPL
}

static IN_PROC processors[] = {
    [IN_NONE] = proc_none,
    [IN_NOP] = proc_nop,
    [IN_LD] = proc_ld,
    [IN_LDH] = proc_ldh,
    [IN_JP] = proc_jp,
    [IN_JR] = proc_jr,
    [IN_CALL] = proc_call,
    [IN_RST] = proc_rst,
    [IN_RET] = proc_ret,
    [IN_RETI] = proc_reti,
    [IN_DI] = proc_di,
    [IN_POP] = proc_pop,
    [IN_PUSH] = proc_push,
    [IN_ADD] = proc_add,
    [IN_ADC] = proc_adc,
    [IN_INC] = proc_inc,
    [IN_DEC] = proc_dec,
    [IN_SUB] = proc_sub,
    [IN_SBC] = proc_sbc,
    [IN_AND] = proc_and,
    [IN_OR] = proc_or,
    [IN_XOR] = proc_xor,
    [IN_CP] = proc_cp,
    [IN_CB] = proc_cb,
};

IN_PROC inst_get_processor(instruction_type type)
{
    return processors[type];
}