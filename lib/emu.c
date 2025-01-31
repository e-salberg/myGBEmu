#include <stdio.h>
#include <emu.h>

static emu_context ctx;

emu_context *emu_get_context() 
{
    return &ctx;
}

void delay(uint32_t ms) 
{
    //SDL delay?
}

int emu_run(int argc, char**argv) 
{
    if (argc < 2) 
    {
        printf("Usage: emu <rom_file>\n");
        return -1;
    }

    cpu_init();

    ctx.running = true;
    ctx.paused = false;
    ctx.ticks = 0;

    while(ctx.running) 
    {
        if (ctx.paused) 
        {
            //delay 10 ms
            delay(10);
            continue;
        }

        if (!cpu_step())
        {
            printf("CPU Stopped\n");
            return -3;
        }
        ctx.ticks++;
    }
}