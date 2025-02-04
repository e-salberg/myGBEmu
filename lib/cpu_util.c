#include <cpu.h>

extern cpu_context ctx;

uint16_t reverse(uint16_t n)
{
    return ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
}

uint16_t cpu_read_reg(reg_type rt)
{
    switch(rt)
    {
        case RT_A:
            return ctx.regs.a;
        case RT_F:
            return ctx.regs.f;
        case RT_B:
            return ctx.regs.b;
        case RT_C:
            return ctx.regs.c;
        case RT_D:
            return ctx.regs.d;
        case RT_E:
            return ctx.regs.e;
        case RT_H:
            return ctx.regs.h;
        case RT_L:
            return ctx.regs.l;
        // not sure why these need to be reversed
        // is it big-endian vs little-endian?
        case RT_AF:
            return reverse(*((uint16_t *)&ctx.regs.a));
        case RT_BC:
            return reverse(*((uint16_t *)&ctx.regs.b));
        case RT_DE:
            return reverse(*((uint16_t *)&ctx.regs.d));
        case RT_HL:
            return reverse(*((uint16_t *)&ctx.regs.h));
        case RT_PC:
            return ctx.regs.pc;
        case RT_SP:
            return ctx.regs.sp;
        default:
            return 0;
    }
}

void cpu_set_reg(reg_type rt, uint16_t val)
{
    switch(rt)
    {
        case RT_A:
            ctx.regs.a = val & 0xFF;
            break;
        case RT_F:
            ctx.regs.f = val & 0xFF;
            break;
        case RT_B:
            ctx.regs.b = val & 0xFF;
            break;
        case RT_C:
            ctx.regs.c = val & 0xFF;
            break;
        case RT_D:
            ctx.regs.d = val & 0xFF;
            break;
        case RT_E:
            ctx.regs.e = val & 0xFF;
            break;
        case RT_H:
            ctx.regs.h = val & 0xFF;
            break;
        case RT_L:
            ctx.regs.l = val & 0xFF;
            break;
        case RT_AF: 
            *((uint16_t *)&ctx.regs.a) = reverse(val);
            break;
        case RT_BC: 
            *((uint16_t *)&ctx.regs.b) = reverse(val);
            break;
        case RT_DE: 
            *((uint16_t *)&ctx.regs.d) = reverse(val);
            break;
        case RT_HL: 
            *((uint16_t *)&ctx.regs.h) = reverse(val);
            break;
        case RT_PC:
            ctx.regs.pc = val;
            break;
        case RT_SP:
            ctx.regs.sp = val;
            break;
        case RT_NONE:
            break;
    }
}