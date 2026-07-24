#include "libc/string.h"
#include "libc/mem.h"
#include "libc/log.h"
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
#include "kernel/drivers/serial.h"

#include "kernel/fs/vfs.h"
#include "kernel/fs/ramfs.h"
#include "kernel/fs/mount.h"
#include "kernel/fs/fat.h"
#include "kernel/fs/ext2.h"
#include "kernel/fs/install.h"

#include "kernel/sched/sched.h"
#include "kernel/proc/proc.h"

#include "../user/init_elf.h"
#include "../user/init_tar.h"

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
    TTY_clear();
    sched_start();
}