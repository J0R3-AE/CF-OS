#include <stdint.h>
#include "kernel/drivers/ps2/ps2.h"
#include "kernel/drivers/ps2/i8042.h"

static void (*keyboard_cb)(uint8_t scancode) = 0;

void ps2_register_keyboard_callback(void (*cb)(uint8_t))
{
    keyboard_cb = cb;
}

void ps2_irq_keyboard(registers_t *regs)
{
    (void)regs;
    uint8_t sc = i8042_read_data();
    if (keyboard_cb)
        keyboard_cb(sc);
}

void ps2_init(void)
{
    i8042_init();

    // Enable first PS/2 port (keyboard)
    i8042_write_command(0xAE);

    // Enable keyboard scanning
    i8042_write_data(0xF4);
}
