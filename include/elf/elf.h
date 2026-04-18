#pragma once
/**
 * @file elf.h
 * @brief ELF32 loader structures, constants, and loading helpers.
 *
 * This header defines:
 * - ELF32 base integer types
 * - ELF32 file header (Elf32_Ehdr)
 * - ELF32 program header (Elf32_Phdr)
 * - magic values, class, machine, and segment constants
 * - validation and loading routines
 *
 * The loader reads an ELF32 image from a vnode and maps its loadable
 * segments into a provided page directory. Used for loading user programs
 * or kernel modules depending on system design.
 */

#include "libk/types.h"
#include "mm/paging.h"
#include "fs/vfs.h" /* for struct vnode */

/* -------------------------------------------------------------------------- */
/*  ELF32 Base Types                                                          */
/* -------------------------------------------------------------------------- */

typedef u32 Elf32_Addr;
typedef u16 Elf32_Half;
typedef u32 Elf32_Off;
typedef s32 Elf32_Sword;
typedef u32 Elf32_Word;

/* -------------------------------------------------------------------------- */
/*  ELF32 File Header                                                         */
/* -------------------------------------------------------------------------- */

/**
 * @struct Elf32_Ehdr
 * @brief ELF32 file header.
 *
 * Contains metadata describing the ELF file layout:
 * - entry point
 * - program header offset
 * - section header offset
 * - machine type
 * - ELF identification bytes
 */
typedef struct
{
    u8 e_ident[16]; /**< Magic, class, data encoding, version. */
    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;
    Elf32_Addr e_entry; /**< Entry point virtual address. */
    Elf32_Off e_phoff;  /**< Program header table offset. */
    Elf32_Off e_shoff;  /**< Section header table offset. */
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;
    Elf32_Half e_phentsize;
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;
    Elf32_Half e_shnum;
    Elf32_Half e_shstrndx;
} Elf32_Ehdr;

/* -------------------------------------------------------------------------- */
/*  ELF32 Program Header                                                      */
/* -------------------------------------------------------------------------- */

/**
 * @struct Elf32_Phdr
 * @brief ELF32 program header.
 *
 * Describes a loadable segment:
 * - file offset
 * - virtual address
 * - memory size
 * - flags (R/W/X)
 */
typedef struct
{
    Elf32_Word p_type;
    Elf32_Off p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
} Elf32_Phdr;

/* -------------------------------------------------------------------------- */
/*  ELF Identification Indexes                                                */
/* -------------------------------------------------------------------------- */

#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5

/* -------------------------------------------------------------------------- */
/*  ELF Magic                                                                 */
/* -------------------------------------------------------------------------- */

#define ELFMAG0 0x7F
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

/* -------------------------------------------------------------------------- */
/*  ELF Class / Data Encoding                                                 */
/* -------------------------------------------------------------------------- */

#define ELFCLASS32 1
#define ELFDATA2LSB 1

/* -------------------------------------------------------------------------- */
/*  ELF Type / Machine                                                        */
/* -------------------------------------------------------------------------- */

#define ET_EXEC 2
#define EM_386 3

/* -------------------------------------------------------------------------- */
/*  Program Header Types & Flags                                              */
/* -------------------------------------------------------------------------- */

#define PT_NULL 0
#define PT_LOAD 1

#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

/* -------------------------------------------------------------------------- */
/*  ELF Loader API                                                            */
/* -------------------------------------------------------------------------- */

/**
 * @brief Validate an ELF32 header.
 *
 * Checks:
 * - magic bytes
 * - class (ELFCLASS32)
 * - data encoding (ELFDATA2LSB)
 * - machine type (EM_386)
 * - version fields
 *
 * @param eh ELF header to validate.
 * @return 0 on success, <0 on invalid ELF.
 */
int elf32_validate(const Elf32_Ehdr *eh);

/**
 * @brief Load an ELF32 image into a page directory.
 *
 * Reads program headers from the vnode, maps loadable segments into the
 * provided address space, and returns the entry point.
 *
 * @param pd         Target page directory.
 * @param vn         Vnode for the ELF file.
 * @param entry_out  Output: entry point virtual address.
 * @return 0 on success, <0 on failure.
 */
int elf32_load_image(struct page_directory *pd, struct vnode *vn, Elf32_Addr *entry_out);

/* ELF_H */
