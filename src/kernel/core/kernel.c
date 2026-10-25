#include "libc/string.h"
#include "libc/mem.h"
#include "libc/types.h"

#include "kernel/arch/multiboot.h"
#include "kernel/arch/io.h"
#include "kernel/arch/gdt.h"
#include "kernel/arch/tss.h"
#include "kernel/arch/idt.h"
#include "kernel/arch/pic.h"
#include "kernel/arch/pit.h"

#include "kernel/mm/vmm.h"
#include "kernel/mm/pmm.h"
#include "kernel/mm/heap.h"
#include "kernel/mm/paging.h"

#include "kernel/ipc/ipc.h"
#include "kernel/net/net.h"
#include "kernel/net/loopback.h"

#include "kernel/drivers/tty.h"
#include "kernel/drivers/ata.h"
#include "kernel/drivers/framebuffer.h"
//#include "kernel/drivers/wm.h"
#include "kernel/drivers/serial.h"
#include "kernel/drivers/ps2/ps2.h"
#include "kernel/drivers/keyboard.h"

#include "kernel/fs/vfs.h"
#include "kernel/fs/ramfs.h"
#include "kernel/fs/mount.h"
#include "kernel/fs/fat.h"
#include "kernel/fs/ext2.h"
#include "kernel/fs/install.h"

#include "kernel/sched/scheduler.h"
#include "kernel/proc/process.h"

#define STACK_SIZE       4096
#define FB_VIRT          0xE0000000

#define HEAP_START       0x00800000
#define HEAP_MAX         0x02000000

#define TOTAL_RAM        (128 * 1024 * 1024)
#define KERNEL_END       0x00200000
#define KERNEL_STACK_TOP 0xC03FF000

static uint8_t kernel_stack[STACK_SIZE];

extern Link g_fs_types;

extern void kernel_init(void);
extern void shell_run(void);
extern void arch_detect(uint32_t *arch, uint32_t *isa);

multiboot_info_t *g_mbi = NULL;

// Kernel entry
void kmain(u32 magic, multiboot_info_t *mbi)
{
    (void)magic;

    uint32_t arch = 0;
    uint32_t isa = 0;

    g_mbi = mbi;

    /* Architecture detection */
    arch_detect(&arch, &isa);

    /* Framebuffer initialization */
    vbe_mode_info_t *vbe =
        (vbe_mode_info_t *)(uintptr_t)mbi->vbe_mode_info;

    uint32_t fb_addr   = vbe->physbase;
    uint32_t fb_width  = vbe->Xres;
    uint32_t fb_height = vbe->Yres;
    uint32_t fb_pitch  = vbe->pitch;
    uint32_t fb_bpp    = vbe->bpp;

    framebuffer_init(
        fb_addr,
        fb_width,
        fb_height,
        fb_pitch,
        fb_bpp
    );

    /* Terminal and window manager */
    TTY_set_fb_backend(1);
    TTY_init();
    //wm_init();

    /* Disable interrupts during early initialization */
    cli();

    /* CPU and interrupt setup */
    gdt_init();

    tss_init(
        (uint32_t)(kernel_stack + sizeof(kernel_stack))
    );

    pic_init();
    idt_init();

    register_interrupt_handler(32, pit_handler);
    register_interrupt_handler(33, ps2_irq_keyboard);

    /* Input and timer setup */
    keyboard_init();
    pit_init(1);

    /* Memory management */
    pmm_init(TOTAL_RAM, KERNEL_END);
    heap_init(HEAP_START, HEAP_MAX, NULL);
    paging_init();

    /* Inter-process communication and networking */
    ipc_init();
    net_init();
    net_loopback_init();

    /* Serial interface */
    serial_init(COM1);

    /* Enable interrupts */
    sti();

    /* Storage devices */
    ata_identify();

    /* Filesystem initialization */
    ListInit(&g_fs_types);

    ramfs_init();
    mount_init();
    fat_init();
    ext2_init();

    /* Process and scheduler initialization */
    proc_init();
    sched_init();

    /* Start with a clean terminal */
    TTY_clear();

    /* Continue kernel initialization */
    kernel_init();

    /* Halt if kernel_init returns */
    for (;;)
    {
        __asm__ volatile("hlt");
    }
}