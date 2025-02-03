#include <stdio.h>
#include <emu.h>
#include <cpu.h>
#include <cartridge.h>

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
        printf("Error: Need to provide a rom file\n");
        return -1;
    }

    if (!load_cartridge(argv[1]))
    {
        printf("Failed to load ROM file: %s\n", argv[1]);
    }

    printf("Cartridge loaded..\n");
    
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

void emu_cycles(int cpu_cycles)
{
    // TODO...
}