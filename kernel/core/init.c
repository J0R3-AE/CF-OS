#include "init.h"

#include "libk/string.h"
#include "libk/mem.h"
#include "libk/log.h"

#include "arch/multiboot.h"
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

#include "elf/elf.h"

#include "sched/sched.h"
#include "proc/proc.h"

static uint8_t kernel_stack[4096];
extern void enter_user_mode(uint32_t entry, uint32_t esp);

extern multiboot_info_t *g_mbi;

/* if you are using initramfs later */
void arch_init(void)
{
    klog_debug("Arch Init Starting...");
    io_disableinterrupts();
    klog_debug("Interrupts Disabled");

    gdt_init();
    klog_debug("Gdt Initialized");

    tss_init((uint32_t)(kernel_stack + sizeof(kernel_stack)));
    klog_debug("Tss Initialized");

    pic_init();
    klog_debug("Pic Initialized");

    idt_init();
    register_interrupt_handler(32, pit_handler);
    register_interrupt_handler(33, keyboard_irq_handler);
    klog_debug("Idt Initialized");

    pit_init(100);
    klog_debug("Pit Initialized");

    pmm_init(TOTAL_RAM, KERNEL_END);
    klog_debug("Pmm Initialized");

    heap_init(HEAP_START, HEAP_MAX, NULL);
    klog_debug("Heap Initialized");

    ipc_init();
    klog_debug("IPC Initialized");

    net_init();
    klog_debug("Net Initialized");

    net_loopback_init();
    klog_debug("Loopback Initialized");

    paging_init();
    klog_debug("Paging Initialized");

    io_enableinterrupts();
    klog_debug("Interrupts Enabled");

    ata_identify();

    ramfs_init();
    klog_debug("Ramfs Initialized");

    mount_init();
    klog_debug("Mount Initialized");

    fat_init();
    klog_debug("Fat Initialized");

    ext2_init();
    klog_debug("Ext2 Initialized");

    proc_init();
    klog_debug("Process Management Initialized");
}