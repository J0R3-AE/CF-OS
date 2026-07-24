#ifndef I8042_H
#define I8042_H

#include <stdint.h>

void i8042_init(void);
void i8042_write_command(uint8_t cmd);
void i8042_write_data(uint8_t data);
uint8_t i8042_read_data(void);

#endif
