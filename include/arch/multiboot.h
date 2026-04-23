/**
 * @file multiboot.h
 * @brief Multiboot 1 boot information structures and VBE mode info.
 *
 * These structures describe the memory layout, modules, framebuffer,
 * and bootloader‑provided metadata passed to the kernel by a Multiboot‑
 * compliant bootloader (e.g., GRUB). The kernel uses this information
 * during early initialization to configure memory management and video
 * output.
 */

#pragma once
#include "libk/types.h"

/* -------------------------------------------------------------------------- */
/* Multiboot Information Structure                                             */
/* -------------------------------------------------------------------------- */

/**
 * @struct multiboot_info
 * @brief Multiboot 1 information structure provided by the bootloader.
 *
 * The fields present depend on the `flags` bitmask. Only fields whose
 * corresponding flag bits are set should be accessed.
 *
 * Important fields:
 *  - mem_lower / mem_upper: conventional + extended memory
 *  - mmap_addr / mmap_length: physical memory map entries
 *  - mods_addr / mods_count: loaded modules (e.g., initrd)
 *  - vbe_*: VESA BIOS Extensions (video mode) information
 */
typedef struct multiboot_info
{
    u32 flags; /**< Bitmask indicating which fields are valid. */

    u32 mem_lower; /**< Lower memory (KB). */
    u32 mem_upper; /**< Upper memory (KB). */

    u32 boot_device; /**< BIOS boot device. */
    u32 cmdline;     /**< Kernel command line (physical address). */

    u32 mods_count; /**< Number of loaded modules. */
    u32 mods_addr;  /**< Physical address of module list. */

    u32 syms[4]; /**< ELF section header table (if present). */

    u32 mmap_length; /**< Length of memory map. */
    u32 mmap_addr;   /**< Physical address of memory map. */

    u32 drives_length; /**< BIOS drive info length. */
    u32 drives_addr;   /**< Physical address of drive info. */

    u32 config_table;     /**< ROM configuration table. */
    u32 boot_loader_name; /**< Bootloader name string. */

    u32 apm_table; /**< APM table (if present). */

    u32 vbe_control_info; /**< Physical address of VBE control info. */
    u32 vbe_mode_info;    /**< Physical address of VBE mode info. */

    u16 vbe_mode;          /**< Current VBE mode. */
    u16 vbe_interface_seg; /**< VBE interface segment. */
    u16 vbe_interface_off; /**< VBE interface offset. */
    u16 vbe_interface_len; /**< VBE interface length. */
} multiboot_info_t;

/* -------------------------------------------------------------------------- */
/* VBE Mode Information Structure                                              */
/* -------------------------------------------------------------------------- */

/**
 * @struct vbe_mode_info
 * @brief VESA BIOS Extensions (VBE) mode information block.
 *
 * This structure describes the framebuffer layout, color masks, memory
 * model, and resolution of the active VBE graphics mode. It is used by
 * the kernel to configure its framebuffer console or graphical output.
 */
typedef struct vbe_mode_info
{
    u16 attributes;         /**< Mode attributes. */
    u8 winA, winB;          /**< Window A/B attributes. */
    u16 granularity;        /**< Window granularity. */
    u16 winsize;            /**< Window size. */
    u16 segmentA, segmentB; /**< Window segments. */
    u32 realFctPtr;         /**< Real‑mode function pointer. */
    u16 pitch;              /**< Bytes per scanline. */

    u16 Xres;        /**< Horizontal resolution. */
    u16 Yres;        /**< Vertical resolution. */
    u8 Xchar;        /**< Character cell width. */
    u8 Ychar;        /**< Character cell height. */
    u8 planes;       /**< Number of memory planes. */
    u8 bpp;          /**< Bits per pixel. */
    u8 banks;        /**< Number of banks. */
    u8 memory_model; /**< Memory model type. */
    u8 bank_size;    /**< Bank size in KB. */
    u8 image_pages;  /**< Number of images. */
    u8 reserved0;

    u8 red_mask;               /**< Red mask size. */
    u8 red_position;           /**< Red mask position. */
    u8 green_mask;             /**< Green mask size. */
    u8 green_position;         /**< Green mask position. */
    u8 blue_mask;              /**< Blue mask size. */
    u8 blue_position;          /**< Blue mask position. */
    u8 rsv_mask;               /**< Reserved mask size. */
    u8 rsv_position;           /**< Reserved mask position. */
    u8 directcolor_attributes; /**< Direct color mode attributes. */

    u32 physbase;       /**< Physical framebuffer address. */
    u32 offscreen;      /**< Offscreen memory offset. */
    u16 offscreen_size; /**< Offscreen memory size. */
} __attribute__((packed)) vbe_mode_info_t;
