#include "kernel/drivers/ps2/ps2.h"
#include "kernel/drivers/ps2/i8042.h"
#include "libc/errno.h"

#define I8042_STATUS_AUXILIARY 0x20
#define I8042_STATUS_TIMEOUT 0x40
#define I8042_STATUS_PARITY 0x80

static void (*keyboard_callback)(u8 scancode);

void ps2_register_keyboard_callback(void (*callback)(u8))
{
    keyboard_callback = callback;
}

static int keyboard_command(u8 command)
{
    for (u32 attempt = 0; attempt < 3; attempt++)
    {
        u8 response;

        if (i8042_write_data(command) < 0)
            return ERR_TIMED_OUT;

        for (u32 poll = 0; poll < 100000; poll++)
        {
            if (!i8042_try_read_data(&response))
                continue;
            if (response == 0xFA)
                return 0;
            if (response == 0xFE)
                break;
        }
    }
    return ERR_TIMED_OUT;
}

int ps2_init(void)
{
    u8 config;

    if (i8042_init() < 0)
        return ERR_TIMED_OUT;
    if (i8042_write_command(0xAE) < 0 ||
        i8042_write_command(0x20) < 0 ||
        i8042_read_data(&config) < 0)
        return ERR_TIMED_OUT;

    config |= 0x41;
    config &= (u8)~0x10;
    if (i8042_write_command(0x60) < 0 ||
        i8042_write_data(config) < 0)
        return ERR_TIMED_OUT;

    if (keyboard_command(0xF5) < 0 ||
        keyboard_command(0xF4) < 0)
        return ERR_TIMED_OUT;

    return 0;
}

void ps2_irq_handler(registers_t *regs)
{
    u8 status, scancode;
    (void)regs;

    status = i8042_read_status();
    if (!(status & 0x01) ||
        (status & (I8042_STATUS_AUXILIARY |
                   I8042_STATUS_TIMEOUT |
                   I8042_STATUS_PARITY)))
    {
        if (status & 0x01)
            (void)i8042_try_read_data(&scancode);
        return;
    }

    if (i8042_try_read_data(&scancode) && keyboard_callback)
        keyboard_callback(scancode);
}
