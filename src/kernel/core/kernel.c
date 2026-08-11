#include "libc/string.h"
#include "libc/mem.h"
#include "libc/log.h"
#include "libc/types.h"
#include "libc/printf.h"

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
#include "kernel/drivers/serial.h"
#include "kernel/drivers/ps2/ps2.h"
#include "kernel/drivers/keyboard.h"

#include "kernel/fs/vfs.h"
#include "kernel/fs/ramfs.h"
#include "kernel/fs/mount.h"
#include "kernel/fs/fat.h"
#include "kernel/fs/ext2.h"
#include "kernel/fs/install.h"

#include "kernel/sched/sched.h"
#include "kernel/proc/proc.h"

extern void kernel_init(void);

#define STACK_SIZE 4096
#define FB_VIRT 0xE0000000

#define HEAP_START 0x00800000 // 8MB
#define HEAP_MAX 0x02000000   // 32MB

#define TOTAL_RAM (128 * 1024 * 1024) // QEMU default
#define KERNEL_END 0x00200000         // ~2MB
#define KERNEL_STACK_TOP 0xC03FF000

#define STACK_SIZE 4096

static uint8_t kernel_stack[4096];

extern Link g_fs_types; /* intrusive FS registry list head */
extern void shell_run(void);
extern void arch_detect(uint32_t *arch, uint32_t *isa);
extern void print_cpu_features(uint32_t arch, uint32_t f);

multiboot_info_t *g_mbi = NULL;

void kmain(u32 magic, multiboot_info_t *mbi)
{
    (void)magic;
    uint32_t arch = 0;
    uint32_t isa = 0;

    g_mbi = mbi;
    
    /* Initialize serial FIRST, before any printf() calls */
    serial_init(0x3F8);
    
    printf("Kernel starting...");

    arch_detect(&arch, &isa);
    print_cpu_features(arch, isa);

    vbe_mode_info_t *vbe = (vbe_mode_info_t *)(uintptr_t)mbi->vbe_mode_info;

    u32 fb_addr = vbe->physbase;
    u32 fb_width = vbe->Xres;
    u32 fb_height = vbe->Yres;
    u32 fb_pitch = vbe->pitch;
    u32 fb_bpp = vbe->bpp;

    framebuffer_init(fb_addr, fb_width, fb_height, fb_pitch, fb_bpp);

    TTY_set_fb_backend(1);

    TTY_init();

    printf("Welcome to AescOS!\n");

    log_set_hide_info(0);
    log_set_hide_all(1);

    disableinterrupts();
    KLOG_LOG("Kernel initializing...");

    gdt_init();
    KLOG_LOG("GDT initialized");

    tss_init((uint32_t)(kernel_stack + sizeof(kernel_stack)));
    KLOG_LOG("TSS initialized");

    pic_init();
    KLOG_LOG("PIC initialized");

    idt_init();
    KLOG_LOG("IDT initialized");
    register_interrupt_handler(32, pit_handler);
    KLOG_LOG("PIT handler registered");

    keyboard_init();
    register_interrupt_handler(33, ps2_irq_keyboard);
    KLOG_LOG("Keyboard handler registered");

    pit_init(1); // 1MHz for now, we'll reprogram it later in sched_init
    KLOG_LOG("PIT initialized");

    pmm_init(TOTAL_RAM, KERNEL_END);
    KLOG_LOG("PMM initialized with %u bytes total RAM", TOTAL_RAM);

    heap_init(HEAP_START, HEAP_MAX, NULL);
    KLOG_LOG("Heap initialized from 0x%x to 0x%x", HEAP_START, HEAP_MAX);

    ipc_init();
    KLOG_LOG("IPC initialized");

    net_init();
    KLOG_LOG("Network stack initialized");

    net_loopback_init();
    KLOG_LOG("Loopback network interface initialized");

    paging_init();
    KLOG_LOG("Paging initialized");

    serial_init(COM1);
    KLOG_LOG("Serial port initialized on COM1");

    enableinterrupts();
    KLOG_LOG("Interrupts enabled");

    ata_identify();
    KLOG_LOG("ATA devices identified");

    ListInit(&g_fs_types);
    KLOG_INFO("Filesystem registry initialized");

    ramfs_init();
    KLOG_LOG("RAMFS initialized");

    mount_init();
    KLOG_LOG("Mount subsystem initialized");

    fat_init();
    KLOG_LOG("FAT filesystem support initialized");

    ext2_init();
    KLOG_LOG("EXT2 filesystem support initialized");

    proc_init();
    KLOG_LOG("Process subsystem initialized");

    sched_init();
    KLOG_LOG("Scheduler initialized");

    TTY_clear();

    kernel_init();

    for (;;)
    {
        printf("kmain: reached idle loop, halting");
        __asm__ volatile("hlt");
    }
}
