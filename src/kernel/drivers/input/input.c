#include "kernel/drivers/input.h"
#include "kernel/drivers/keyboard.h"
#include "kernel/drivers/line.h"
#include "kernel/drivers/ps2/ps2.h"

int input_init(void)
{
    keyboard_init();
    return ps2_init();
}

void input_handle_ascii(u8 character)
{
    line_handle_ascii(character);
}
