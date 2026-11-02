#ifndef I8042_H
#define I8042_H

#include "libc/types.h"

int i8042_init(void);
int i8042_write_command(u8 command);
int i8042_write_data(u8 data);
int i8042_read_data(u8 *data);
u8 i8042_read_status(void);
bool i8042_try_read_data(u8 *data);

#endif
