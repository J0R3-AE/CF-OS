#include "libk/string.h"
#include "libk/mem.h"
#include "libk/log.h"
#include "libk/types.h"

#include "arch/multiboot.h"
#include "arch/io.h"
#include "arch/gdt.h"
#include "arch/tss.h"
#include "arch/idt.h"
#include "arch/pic.h"
#include "arch/pit.h"

#include "mm/vmm.h"
#include "mm/pmm.h"
#include "mm/heap.h"
#include "mm/paging.h"

#include "ipc/ipc.h"
#include "net/net.h"
#include "net/loopback.h"

#include "drivers/tty.h"
#include "drivers/kbd.h"
#include "drivers/ata.h"
#include "drivers/fbcon.h"
#include "drivers/keyboard.h"

#include "fs/vfs.h"
#include "fs/ramfs.h"
#include "fs/mount.h"
#include "fs/fat.h"
#include "fs/ext2.h"

#include "sched/sched.h"
#include "proc/proc.h"



#define HEAP_START  0x00800000   // 8MB
#define HEAP_MAX    0x02000000   // 32MB

#define TOTAL_RAM        (128 * 1024 * 1024)   // QEMU default
#define KERNEL_END       0x00200000            // ~2MB
#define KERNEL_STACK_TOP 0xC03FF000

#define STACK_SIZE 4096

static uint8_t kernel_stack[4096];

extern Link g_fs_types; /* intrusive FS registry list head */ 

void kernel_init(void)
{
    klog_log("Arch Init Starting...");
    io_disableinterrupts();
    klog_log("Interrupts Disabled");

    gdt_init();
    klog_log("Gdt Initialized");

    tss_init((uint32_t)(kernel_stack + sizeof(kernel_stack)));
    klog_log("Tss Initialized");

    pic_init();
    klog_log("Pic Initialized");

    idt_init();
    register_interrupt_handler(32, pit_handler);
    register_interrupt_handler(33, keyboard_irq_handler);
    klog_log("Idt Initialized");

    pit_init(100);
    klog_log("Pit Initialized");

    pmm_init(TOTAL_RAM, KERNEL_END);
    klog_log("Pmm Initialized");

    heap_init(HEAP_START, HEAP_MAX, NULL);
    klog_log("Heap Initialized");

    ipc_init();
    klog_log("IPC Initialized");

    net_init();
    klog_log("Net Initialized");

    net_loopback_init();
    klog_log("Loopback Initialized");

    paging_init();
    klog_log("Paging Initialized");

    io_enableinterrupts();
    klog_log("Interrupts Enabled");

    ata_identify();

    ListInit(&g_fs_types);

    ramfs_init();
    klog_log("Ramfs Initialized");

    mount_init();
    klog_log("Mount Initialized");

    fat_init();
    klog_log("Fat Initialized");

    ext2_init();
    klog_log("Ext2 Initialized");
}