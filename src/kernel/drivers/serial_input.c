/* kernel/drivers/serial_input.c
 * Poll COM1 for incoming characters and feed them into the keyboard buffer
 * so serial stdin (when QEMU is run with `-serial stdio`) can be used as
 * the system console input.
 */

#include "drivers/kbd.h"
#include "sched/sched.h"
#include "arch/io.h"
#include "libk/log.h"

#define COM1_PORT 0x3F8

void serial_input_thread(void *arg)
{
    (void)arg;

    KLOG_INFO("serial_input_thread: started");

    for (;;)
    {
        u8 status = io_Read8(COM1_PORT + 5);
        if (status & 0x01)
        {
            u8 data = io_Read8(COM1_PORT);
            /* Push ASCII directly into kbd buffer so existing input code works */
            kbd_push((int)data);
        }
        else
        {
            ksched_yield();
        }
    }
}
