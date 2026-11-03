#include "libc/string.h"
#include "libc/mem.h"
#include "kernel/log.h"
#include "kernel/printf.h"
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

#include "kernel/drivers/console.h"
#include "kernel/drivers/tty.h"
#include "kernel/drivers/ata.h"
#include "kernel/drivers/framebuffer.h"
#include "kernel/drivers/serial.h"
#include "kernel/drivers/input.h"
#include "kernel/drivers/ps2/ps2.h"

#include "kernel/fs/ramfs.h"
#include "kernel/fs/mount.h"
#include "kernel/fs/partition.h"
#include "kernel/fs/fat.h"
#include "kernel/fs/ext2.h"
#include "kernel/fs/blockdev.h"

#include "kernel/sched/sched.h"
#include "kernel/proc/proc.h"

extern void kernel_init(void);
extern void shell_run(void);

#define FB_VIRT 0xE0000000
#define FB_VIRT_LIMIT 0xE1000000
#define MULTIBOOT_INFO_VBE 0x00000800

#define HEAP_START 0x00800000 // 8MB
#define HEAP_MAX 0x02000000   // 32MB

#define TOTAL_RAM (128 * 1024 * 1024) // QEMU default
#define KERNEL_END 0x00200000         // ~2MB
static uint8_t kernel_stack[4096];

extern Link g_fs_types; /* intrusive FS registry list head */

multiboot_info_t *g_mbi = NULL;

typedef struct
{
    u32 physical;
    u32 width;
    u32 height;
    u32 pitch;
    u32 bpp;
    bool valid;
} boot_framebuffer_t;

static boot_framebuffer_t read_boot_framebuffer(multiboot_info_t *mbi)
{
    boot_framebuffer_t framebuffer = {0};
    vbe_mode_info_t *mode;

    if (!mbi || !(mbi->flags & MULTIBOOT_INFO_VBE) || !mbi->vbe_mode_info)
        return framebuffer;

    mode = (vbe_mode_info_t *)(uptr)mbi->vbe_mode_info;
    if ((mode->attributes & 0x91) != 0x91 ||
        mode->planes != 1 || mode->memory_model != 6 || !mode->physbase ||
        !mode->Xres || !mode->Yres || !mode->pitch ||
        (mode->bpp != 16 && mode->bpp != 24 && mode->bpp != 32))
        return framebuffer;
    if ((mode->bpp == 16 &&
         (mode->red_mask != 5 || mode->red_position != 11 ||
          mode->green_mask != 6 || mode->green_position != 5 ||
          mode->blue_mask != 5 || mode->blue_position != 0)) ||
        (mode->bpp != 16 &&
         (mode->red_mask != 8 || mode->red_position != 16 ||
          mode->green_mask != 8 || mode->green_position != 8 ||
          mode->blue_mask != 8 || mode->blue_position != 0)))
        return framebuffer;

    framebuffer.physical = mode->physbase;
    framebuffer.width = mode->Xres;
    framebuffer.height = mode->Yres;
    framebuffer.pitch = mode->pitch;
    framebuffer.bpp = mode->bpp;
    framebuffer.valid = true;
    return framebuffer;
}

static bool map_boot_framebuffer(const boot_framebuffer_t *framebuffer)
{
    u64 page_start;
    u64 byte_count;
    u64 map_size;
    u64 page_count;

    if (!framebuffer || !framebuffer->valid)
        return false;

    byte_count = (u64)framebuffer->pitch * framebuffer->height;
    page_start = framebuffer->physical & ~(u64)(PAGE_SIZE - 1);
    map_size = byte_count + (framebuffer->physical - page_start);
    if (!byte_count || map_size > (FB_VIRT_LIMIT - FB_VIRT) ||
        (u64)framebuffer->physical + byte_count > 0x100000000ULL)
        return false;

    page_count = (map_size + PAGE_SIZE - 1) / PAGE_SIZE;
    for (u64 page = 0; page < page_count; page++)
    {
        u32 virtual_address = FB_VIRT + (u32)(page * PAGE_SIZE);
        u32 physical_address = (u32)(page_start + page * PAGE_SIZE);
        u32 directory_entry = kernel_pd->entries[virtual_address >> 22];
        u32 *page_table;
        u32 table_entry;

        if (directory_entry & PAGE_PRESENT)
        {
            page_table = (u32 *)(directory_entry & ~0xFFFu);
            table_entry = page_table[(virtual_address >> 12) & 0x3FF];
            if (!(table_entry & PAGE_PRESENT))
                paging_map_page(kernel_pd, virtual_address, physical_address,
                                PAGE_PRESENT | PAGE_RW);
        }
        else
            paging_map_page(kernel_pd, virtual_address, physical_address,
                            PAGE_PRESENT | PAGE_RW);

        directory_entry = kernel_pd->entries[virtual_address >> 22];
        if (!(directory_entry & PAGE_PRESENT))
            return false;
        page_table = (u32 *)(directory_entry & ~0xFFFu);
        table_entry = page_table[(virtual_address >> 12) & 0x3FF];
        if (!(table_entry & PAGE_PRESENT) ||
            (table_entry & ~0xFFFu) != physical_address)
            return false;
    }

    if (!console_init_framebuffer(
            FB_VIRT + (framebuffer->physical - (u32)page_start),
            framebuffer->width, framebuffer->height,
            framebuffer->pitch, framebuffer->bpp))
        return false;

    TTY_set_fb_backend(1);
    TTY_init();
    return true;
}

void kmain(u32 magic, multiboot_info_t *mbi)
{
    (void)magic;

    g_mbi = mbi;
    boot_framebuffer_t boot_framebuffer = read_boot_framebuffer(mbi);
    (void)serial_init(COM1);

    console_init_vga();
    TTY_init();
    printf("Kernel starting...\n");
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

    register_interrupt_handler(33, ps2_irq_handler);
    if (input_init() == 0)
    {
        pic_unmask_irq(1);
        KLOG_LOG("PS/2 keyboard initialized");
    }
    else
    {
        KLOG_WARN("PS/2 keyboard initialization failed");
    }

    pit_init(100);
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

    if (map_boot_framebuffer(&boot_framebuffer))
    {
        KLOG_LOG("Boot framebuffer console enabled");
    }
    else
    {
        KLOG_INFO("Using VGA text console");
    }

    enableinterrupts();
    KLOG_LOG("Interrupts enabled");

    if (ata_init() == 0)
    {
        struct blockdev *disk = blockdev_lookup("hd0");
        u8 first_sector[512];

        printf("ATA primary master registered: %u sectors\n",
               ata_sector_count());
        int partition_count = partition_scan(disk);
        if (partition_count >= 0)
            printf("MBR partition scan found %d partitions\n", partition_count);
        else
            printf("MBR partition scan failed: %d\n", partition_count);
        if (blockdev_read(disk, 0, 1, first_sector) == 0)
            printf("ATA read test passed (LBA 0)\n");
        else
            printf("ATA read test failed (LBA 0)\n");

#ifdef ATA_PIO_SELFTEST
        if (ata_selftest() == 0)
            printf("ATA read/write test passed; LBA 0 restored\n");
        else
            printf("ATA read/write test failed; inspect test disk\n");
#endif
    }
    else
    {
        printf("ATA primary master not available\n");
    }

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

    kernel_init();

    printf("kmain: returned from kernel_init, going into backup shell");
    shell_run();

    for (;;)
    {
        printf("kmain: reached idle loop, halting");
        __asm__ volatile("hlt");
    }
}
