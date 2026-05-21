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

#include "../user/init_elf.h"
#include "../user/init_tar.h"
#include "sched/sched.h"
#include "proc/proc.h"

#define HEAP_START 0x00800000 // 8MB
#define HEAP_MAX 0x02000000   // 32MB

#define TOTAL_RAM (128 * 1024 * 1024) // QEMU default
#define KERNEL_END 0x00200000         // ~2MB
#define KERNEL_STACK_TOP 0xC03FF000

#define STACK_SIZE 4096

static uint8_t kernel_stack[4096];

extern Link g_fs_types; /* intrusive FS registry list head */
extern int exec_elf_image(const void *image, u32 size);

void kernel_init(void)
{
    io_disableinterrupts();
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
    register_interrupt_handler(33, keyboard_irq_handler);
    KLOG_LOG("Keyboard handler registered");

    pit_init(0.000001); // 1MHz for now, we'll reprogram it later in sched_init
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

    io_enableinterrupts();
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

    /* Mount an in-memory ramfs as the root filesystem and populate it from
       an embedded tarball if available. If /init exists in the ramfs, use
       that; otherwise fall back to the embedded init ELF image. */
    if (mount_do_mount(&ramfs_type, "", "/", NULL) != 0)
    {
        KLOG_ERROR("Failed to mount ramfs as root");
    }
    else
    {
        struct vnode *root = vfs_get_root();
        if (!root)
        {
            KLOG_ERROR("Mount succeeded but root vnode is NULL");
        }
        else
        {
            KLOG_INFO("Root vnode %p ops=%p type=%d", root, root->ops, root->type);
            if (!root->ops)
                KLOG_ERROR("Root vnode has no ops table");
        }

        if (root && user_init_tar_image_size > 0)
        {
            KLOG_INFO("Extracting embedded init.tar into ramfs (size=%u)", user_init_tar_image_size);
            tar_extract(root, (void *)user_init_tar_image, user_init_tar_image_size);
        }

        KLOG_INFO("Attempting vfs lookup for /init");
        struct vnode *init_vn = NULL;
        int lookup_ret = vfs_lookup("/init", &init_vn);
        if (lookup_ret == 0)
        {
            KLOG_INFO("vfs_lookup(/init) returned vnode %p", init_vn);
            if (exec_elf_vnode(init_vn) == 0)
                KLOG_OKAY("Usermode init process scheduled from /init (tar)");
            else
                KLOG_ERROR("Failed to exec /init from ramfs");
        }
        else
        {
            KLOG_WARN("vfs_lookup(/init) failed with %d, falling back to embedded ELF", lookup_ret);
            /* Fallback to embedded ELF image */
            if (exec_elf_image(user_init_elf_image, user_init_elf_image_size) == 0)
            {
                KLOG_OKAY("Usermode init process scheduled (embedded ELF)");
            }
            else
            {
                KLOG_ERROR("Failed to schedule usermode init process");
            }
        }
    }

    KLOG_LOG("Kernel initialization complete, starting scheduler...");
    sched_start();
}