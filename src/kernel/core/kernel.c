#include "arch/multiboot.h"
#include "arch/io.h"
#include "drivers/fbcon.h"

#include "libk/string.h"
#include "libk/printf.h"
#include "proc/proc.h"
#include "drivers/tty.h"

#include "libk/log.h"
#include "sched/sched.h"

extern void kernel_init(void);

#define STACK_SIZE 4096
#define FB_VIRT 0xE0000000

multiboot_info_t *g_mbi = NULL;

void kmain(u32 magic, multiboot_info_t *mbi)
{
    (void)magic;

    g_mbi = mbi;
    KLOG_LOG("Kernel starting...");

    /* Early devices */
    i386SERIAL_init();

    if (mbi->vbe_mode_info)
    {
        vbe_mode_info_t *vbe = (vbe_mode_info_t *)(uintptr_t)mbi->vbe_mode_info;

        u32 fb_addr = vbe->physbase;
        u32 fb_width = vbe->Xres;
        u32 fb_height = vbe->Yres;
        u32 fb_pitch = vbe->pitch;
        u32 fb_bpp = vbe->bpp;

        KLOG_LOG("FB: addr=%x width=%u height=%u pitch=%u bpp=%u", fb_addr, fb_width, fb_height, fb_pitch, fb_bpp);
        fbcon_init(fb_addr, fb_width, fb_height, fb_pitch, fb_bpp);
        TTY_set_fb_backend(1);
    }

    TTY_init();

    KLOG_LOG("Welcome to MiniOS!");

    log_set_hide_info(1);
    log_set_hide_all(1);

    kernel_init();

    KLOG_FATAL("kmain: returned from kernel_init, halting");

    for (;;)
    {
        KLOG_FATAL("kmain: reached idle loop, halting");
        __asm__ volatile("hlt");
    }
}
