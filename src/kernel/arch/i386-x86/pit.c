/** @author JPEG */

#include "arch/pit.h"

#include "arch/io.h"
#include "arch/pic.h"
#include "libk/printf.h"
#include "libk/types.h"
#include "drivers/keyboard.h"

#include "libk/log.h"


static volatile u32 g_pit_ticks = 0;
static volatile u32 g_pit_frequency = 0;

/* Legacy / compatibility helpers */
static void pit_program(u32 hz)
{
    u16 divisor;
    u8 command;

    if (hz == 0)
    {
        hz = 100;
    }

    /*
     * PIT input clock is 1,193,182 Hz.
     * Divisor must fit in 16 bits, so clamp to a valid range.
     */
    if (hz > 1193182u)
    {
        hz = 1193182u;
    }

    divisor = (u16)(1193182u / hz);
    if (divisor == 0)
    {
        divisor = 1;
    }

    g_pit_frequency = hz;

    /*
     * Channel 0, access mode LSB then MSB, mode 3 (square wave),
     * binary counting.
     */
    command = (u8)(
        PIT_OCW_COUNTER_0 |
        PIT_OCW_RL_DATA |
        PIT_OCW_MODE_SQUAREWAVEGEN |
        PIT_OCW_BINCOUNT_BINARY
    );

    io_Write8(PIT_REG_COMMAND, command);
    io_Write8(PIT_REG_COUNTER0, (u8)(divisor & 0xFF));
    io_Write8(PIT_REG_COUNTER0, (u8)((divisor >> 8) & 0xFF));
}

/* Your old-style IRQ0 handler */
void pit_handler(registers_t *r)
{
    (void)r;

    g_ticks++;

    wake_sleepers();
    sched_tick();
    pic_send_eoi(0);
}

/* New header function */
void pit_init(u32 hz)
{

    pit_program(hz);

    /* Hook IRQ0 -> PIT handler */

    keyboard_init();

    /* Make sure timer IRQ is enabled */
    pic_unmask_irq(0);
}

void pit_log(void)
{
}
/* Optional helpers */
u32 pit_get_ticks(void) { return g_pit_ticks; }
u32 pit_get_frequency(void) { return g_pit_frequency; }
void pit_reset_ticks(void) { g_pit_ticks = 0; }