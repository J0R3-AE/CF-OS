#include <stdint.h>
#include "kernel/drivers/keyboard.h"
#include "kernel/drivers/ps2/ps2.h"

void input_init(void)
{
    ps2_init();
    keyboard_init();
}
