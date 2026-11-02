#include "kernel/drivers/ps2/i8042.h"
#include "kernel/arch/io.h"
#include "libc/errno.h"

#define I8042_DATA_PORT 0x60
#define I8042_STATUS_PORT 0x64
#define I8042_COMMAND_PORT 0x64
#define I8042_OUTPUT_FULL 0x01
#define I8042_INPUT_FULL 0x02
#define I8042_POLL_LIMIT 100000

u8 i8042_read_status(void)
{
    return in8(I8042_STATUS_PORT);
}

bool i8042_try_read_data(u8 *data)
{
    if (!data || !(i8042_read_status() & I8042_OUTPUT_FULL))
        return false;
    *data = in8(I8042_DATA_PORT);
    return true;
}

int i8042_write_command(u8 command)
{
    for (u32 i = 0; i < I8042_POLL_LIMIT; i++)
    {
        if (!(i8042_read_status() & I8042_INPUT_FULL))
        {
            out8(I8042_COMMAND_PORT, command);
            return 0;
        }
    }
    return ERR_TIMED_OUT;
}

int i8042_write_data(u8 data)
{
    for (u32 i = 0; i < I8042_POLL_LIMIT; i++)
    {
        if (!(i8042_read_status() & I8042_INPUT_FULL))
        {
            out8(I8042_DATA_PORT, data);
            return 0;
        }
    }
    return ERR_TIMED_OUT;
}

int i8042_read_data(u8 *data)
{
    if (!data)
        return ERR_INVALID_ARGUMENT;

    for (u32 i = 0; i < I8042_POLL_LIMIT; i++)
    {
        if (i8042_try_read_data(data))
            return 0;
    }
    return ERR_TIMED_OUT;
}

int i8042_init(void)
{
    u8 config;

    if (i8042_write_command(0xAD) < 0 ||
        i8042_write_command(0xA7) < 0)
        return ERR_TIMED_OUT;

    u32 poll;
    for (poll = 0; poll < I8042_POLL_LIMIT; poll++)
    {
        if (!(i8042_read_status() & I8042_OUTPUT_FULL))
            break;
        (void)in8(I8042_DATA_PORT);
    }
    if (poll == I8042_POLL_LIMIT)
        return ERR_TIMED_OUT;

    if (i8042_write_command(0x20) < 0 ||
        i8042_read_data(&config) < 0)
        return ERR_TIMED_OUT;

    config &= (u8)~0x03;
    config |= 0x60;
    config &= (u8)~0x10;
    config |= 0x20;

    if (i8042_write_command(0x60) < 0 ||
        i8042_write_data(config) < 0)
        return ERR_TIMED_OUT;

    return 0;
}
