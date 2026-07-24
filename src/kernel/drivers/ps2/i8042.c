#include <stdint.h>
#include "kernel/drivers/ps2/i8042.h"
#include "kernel/arch/io.h"   // your in8/out8

#define I8042_DATA_PORT   0x60
#define I8042_STATUS_PORT 0x64
#define I8042_CMD_PORT    0x64

#define I8042_STATUS_OBF  0x01
#define I8042_STATUS_IBF  0x02

static void i8042_wait_input_empty(void)
{
    while (in8(I8042_STATUS_PORT) & I8042_STATUS_IBF)
        ;
}

static void i8042_wait_output_full(void)
{
    while (!(in8(I8042_STATUS_PORT) & I8042_STATUS_OBF))
        ;
}

void i8042_write_command(uint8_t cmd)
{
    i8042_wait_input_empty();
    out8(I8042_CMD_PORT, cmd);
}

void i8042_write_data(uint8_t data)
{
    i8042_wait_input_empty();
    out8(I8042_DATA_PORT, data);
}

uint8_t i8042_read_data(void)
{
    i8042_wait_output_full();
    return in8(I8042_DATA_PORT);
}

void i8042_init(void)
{
    // Basic reset/disable could go here if you want
    // For now, minimal stub.
}
