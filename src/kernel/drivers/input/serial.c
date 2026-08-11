#include "kernel/drivers/serial.h"
#include "kernel/arch/io.h"

/*
 * Replace with your own I/O header if available.
 */

static u16 g_serial_port = COM1;
static bool g_initialized = false;

/* -------------------------------------------------------------------------- */
/* Internal Helpers                                                            */
/* -------------------------------------------------------------------------- */

int serial_received(void)
{
    return in8(SERIAL_LINE_STATUS(g_serial_port)) & 1;
}

int serial_transmit_empty(void)
{
    return in8(SERIAL_LINE_STATUS(g_serial_port)) & 0x20;
}

static void serial_wait_tx(void)
{
    /* Wait for transmit empty, but bound the wait to avoid locking
     * the kernel forever if the serial device is not present or not
     * responding (helps when running under different QEMU configs). */
    for (int i = 0; i < 10000 && !serial_transmit_empty(); ++i)
    {
        /* small no-op to prevent optimization */
        __asm__ volatile("");
    }
}

static void serial_wait_rx(void)
{
    while (!serial_received())
    {
    }
}

/* -------------------------------------------------------------------------- */
/* Initialization                                                              */
/* -------------------------------------------------------------------------- */

bool serial_init(u16 port)
{
    g_serial_port = port;

    /* Disable interrupts */
    out8(SERIAL_INTERRUPT(port), 0x00);

    /* Enable DLAB */
    out8(SERIAL_LINE(port), 0x80);

    /*
     * Baud rate divisor = 3
     * 115200 / 3 = 38400 baud
     */
    out8(SERIAL_DIVISOR_LOW(port), 0x03);
    out8(SERIAL_DIVISOR_HIGH(port), 0x00);

    /* 8 bits, no parity, one stop bit */
    out8(SERIAL_LINE(port), 0x03);

    /* Enable FIFO */
    out8(SERIAL_FIFO(port), 0xC7);

    /* IRQs enabled, RTS/DSR set */
    out8(SERIAL_MODEM(port), 0x0B);

    /* Loopback mode for testing */
    out8(SERIAL_MODEM(port), 0x1E);

    out8(SERIAL_DATA(port), 0xAE);

    if (in8(SERIAL_DATA(port)) != 0xAE)
    {
        g_initialized = false;
        return false;
    }

    /* Normal mode */
    out8(SERIAL_MODEM(port), 0x0F);

    g_initialized = true;
    return true;
}

/* -------------------------------------------------------------------------- */
/* Port Management                                                             */
/* -------------------------------------------------------------------------- */

bool serial_is_initialized(void)
{
    return g_initialized;
}

void serial_set_port(u16 port)
{
    g_serial_port = port;
}

u16 serial_get_port(void)
{
    return g_serial_port;
}

/* -------------------------------------------------------------------------- */
/* Output                                                                      */
/* -------------------------------------------------------------------------- */

void serial_write_char(char c)
{
    /* Emit serial output even if loopback test failed. This helps
     * capture early boot messages from QEMU for debugging the
     * bootloop. Remove this override once issue is resolved. */
    serial_wait_tx();
    out8(SERIAL_DATA(g_serial_port), (u8)c);
}

void serial_write(const char *str)
{
    if (!str)
        return;

    while (*str)
    {
        serial_write_char(*str++);
    }
}

void serial_write_line(const char *str)
{
    serial_write(str);
    serial_write("\r\n");
}

void serial_clear(void)
{
    serial_write("\033[2J\033[H");
}

/* -------------------------------------------------------------------------- */
/* Input                                                                       */
/* -------------------------------------------------------------------------- */

char serial_read_char(void)
{
    serial_wait_rx();
    return (char)in8(SERIAL_DATA(g_serial_port));
}

/* -------------------------------------------------------------------------- */
/* Number Printing                                                             */
/* -------------------------------------------------------------------------- */

void serial_write_hex(u32 value)
{
    static const char hex[] = "0123456789ABCDEF";

    serial_write("0x");

    for (int i = 28; i >= 0; i -= 4)
    {
        serial_write_char(hex[(value >> i) & 0xF]);
    }
}

void serial_write_dec(u32 value)
{
    char buffer[16];
    int pos = 0;

    if (value == 0)
    {
        serial_write_char('0');
        return;
    }

    while (value > 0)
    {
        buffer[pos++] = '0' + (value % 10);
        value /= 10;
    }

    while (pos--)
    {
        serial_write_char(buffer[pos]);
    }
}