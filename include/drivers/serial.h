#pragma once

#include <stdint.h>
#include "libk/types.h"

/* -------------------------------------------------------------------------- */
/* COM Port Base Addresses                                                     */
/* -------------------------------------------------------------------------- */

#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8

/* -------------------------------------------------------------------------- */
/* UART Registers                                                              */
/* -------------------------------------------------------------------------- */

#define SERIAL_DATA(port)       (port)
#define SERIAL_INTERRUPT(port)  (port + 1)
#define SERIAL_DIVISOR_LOW(port)  (port)
#define SERIAL_DIVISOR_HIGH(port) (port + 1)
#define SERIAL_FIFO(port)       (port + 2)
#define SERIAL_LINE(port)       (port + 3)
#define SERIAL_MODEM(port)      (port + 4)
#define SERIAL_LINE_STATUS(port)(port + 5)

/* -------------------------------------------------------------------------- */
/* API                                                                         */
/* -------------------------------------------------------------------------- */

bool serial_init(u16 port);

bool serial_is_initialized(void);

void serial_set_port(u16 port);
u16 serial_get_port(void);

void serial_write_char(char c);
void serial_write(const char *str);
void serial_write_line(const char *str);

char serial_read_char(void);

void serial_write_hex(u32 value);
void serial_write_dec(u32 value);

void serial_clear(void);

int serial_received(void);
int serial_transmit_empty(void);