#include "arch/multiboot.h"

#include "libk/string.h"
#include "libk/printf.h"
#include "proc/proc.h"

#include "libk/log.h"
#include "sched/sched.h"

extern void arch_init(void);
extern void shell_main(void);

#define STACK_SIZE 4096
#define FB_VIRT 0xE0000000

multiboot_info_t *g_mbi = NULL;

void kmain(u32 magic, multiboot_info_t *mbi)
{
    (void)magic;

    g_mbi = mbi;
    klog_info("Kernel starting...");

    i386SERIAL_init();

    if (mbi->vbe_mode_info)
    {
        vbe_mode_info_t *vbe = (vbe_mode_info_t *)(uintptr_t)mbi->vbe_mode_info;
        fbcon_init(vbe->physbase, vbe->Xres, vbe->Yres, vbe->pitch, vbe->bpp);
        TTY_set_fb_backend(1);
        TTY_init();
    }

    arch_init();

    sched_init();
    u8 *stack_sh = malloc(STACK_SIZE);

    Thread *shell_thread = thread_create(shell_main, NULL, stack_sh, STACK_SIZE);

    sched_add(shell_thread);

    sched_start();

    for (;;)
        __asm__ volatile("hlt");
}
