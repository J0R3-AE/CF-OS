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
#include "drivers/framebuffer.h"
#include "drivers/keyboard.h"
#include "drivers/serial.h"

#include "fs/vfs.h"
#include "fs/ramfs.h"
#include "fs/mount.h"
#include "fs/fat.h"
#include "fs/ext2.h"
#include "fs/install.h"

#include "../user/init_elf.h"
#include "../user/init_tar.h"
#include "sched/sched.h"
#include "proc/proc.h"

/* serial input thread (feeds COM1 into kbd buffer when -serial stdio used) */
extern void serial_input_thread(void *arg);

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

    /* Try to mount disk root first (installed OS) */
    int disk_mounted = 0;
    if (install_mount_disk_root() == 0)
    {
        KLOG_OKAY("Disk root mounted successfully (installed OS)");
        disk_mounted = 1;
    }
    else
    {
        KLOG_WARN("Disk root not available, falling back to ramfs");
    }

    /* If disk was mounted, make sure it actually contains an init program.
     * If not, unmount the disk and fall back to ramfs so the embedded init
     * or ramfs-provided /init can be used instead of a blank disk.
     */
    if (disk_mounted)
    {
        if (!install_check_disk_installed())
        {
            KLOG_WARN("Disk mounted but no /init found on disk, unmounting and falling back to ramfs");
            if (mount_do_unmount("/") != 0)
                KLOG_WARN("Failed to unmount disk root, continuing with fallback");
            disk_mounted = 0;
        }
        else
        {
            KLOG_INFO("Disk appears to contain installation (found /init)");
        }
    }

    /* If disk not mounted, or if we want live environment, mount ramfs as fallback */
    int ramfs_mounted = 0;
    if (!disk_mounted)
    {
        if (mount_do_mount(&ramfs_type, "", "/", NULL) != 0)
        {
            KLOG_ERROR("Failed to mount ramfs as root");
        }
        else
        {
            ramfs_mounted = 1;
        }
    }

    struct vnode *root = vfs_get_root();
    if (!root)
    {
        KLOG_ERROR("No root filesystem mounted");
    }
    else
    {
        KLOG_INFO("Root vnode %p ops=%p type=%d", root, root->ops, root->type);

        /* If ramfs was mounted, extract embedded tarball */
        if (ramfs_mounted && user_init_tar_image_size > 0)
        {
            KLOG_INFO("Extracting embedded init.tar into ramfs (size=%u)", user_init_tar_image_size);
            tar_extract(root, (void *)user_init_tar_image, user_init_tar_image_size);
        }

        KLOG_INFO("Attempting vfs lookup for /init");
        struct vnode *init_vn = NULL;
        int lookup_ret = vfs_lookup("/init", &init_vn);
        if (lookup_ret == 0 && init_vn)
        {
            KLOG_INFO("vfs_lookup(/init) returned vnode %p", init_vn);
            if (exec_elf_vnode(init_vn) == 0)
            {
                if (disk_mounted)
                    KLOG_OKAY("Usermode init process scheduled from /init (disk)");
                else
                    KLOG_OKAY("Usermode init process scheduled from /init (ramfs)");
            }
            else
                KLOG_ERROR("Failed to exec /init");
        }
        else
        {
            KLOG_WARN("vfs_lookup(/init) failed, falling back to embedded ELF");
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