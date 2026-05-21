#include "elf/elf.h"
#include "fs/vfs.h"
#include "mm/paging.h"
#include "mm/pmm.h"
#include "mm/heap.h"
#include "libk/string.h"
#include "libk/mem.h"
#include "libk/math.h"
#include "libk/log.h"

/*
 * ELF32 loader for MiniOS
 *
 * Notes:
 * - This loader expects a temporary kernel mapping helper for physical frames.
 * - If your paging layer uses a different name, swap these externs.
 * - The loader reads segment data into a kernel buffer first, then copies it
 *   into mapped pages. That keeps the page-loading logic simple and safe.
 */

extern void *paging_temp_map_frame(struct page_directory *pd, u32 phys);
/*
 * Optional helper. If you have an explicit unmap call, use it here.
 * If not, you can leave it as a no-op in paging.c or remove this extern.
 */
extern void paging_temp_unmap_frame(struct page_directory *pd);

typedef struct elf_seg_page
{
    u32 vaddr;
    u32 phys;
} elf_seg_page_t;

static int read_fully(struct vnode *vn, u32 offset, void *buf, u32 size)
{
    if (!vn || !buf)
        return -1;

    KLOG_INFO("read_fully: vnode=%p offset=%u size=%u", vn, offset, size);
    struct file *f = vfs_open_vnode(vn);
    if (!f)
    {
        KLOG_ERROR("read_fully: failed to open vnode %p", vn);
        return -1;
    }

    f->offset = offset;

    usize total = 0;
    while (total < size)
    {
        usize got = 0;
        usize want = (usize)(size - total);

        int r = vfs_read(f, (u8 *)buf + total, want, &got);
        KLOG_INFO("read_fully: vfs_read returned %d got=%u want=%u offset=%u", r, got, want, offset + total);
        if (r != 0)
        {
            vfs_close(f);
            return -1;
        }

        if (got == 0)
        {
            KLOG_ERROR("read_fully: unexpected EOF vnode=%p offset=%u", vn, offset + total);
            vfs_close(f);
            return -1;
        }

        total += got;
    }

    vfs_close(f);
    return 0;
}

int elf32_validate(const Elf32_Ehdr *eh)
{
    if (!eh)
        return -1;

    if (eh->e_ident[EI_MAG0] != ELFMAG0 ||
        eh->e_ident[EI_MAG1] != ELFMAG1 ||
        eh->e_ident[EI_MAG2] != ELFMAG2 ||
        eh->e_ident[EI_MAG3] != ELFMAG3)
        return -1;

    if (eh->e_ident[EI_CLASS] != ELFCLASS32)
        return -1;

    if (eh->e_ident[EI_DATA] != ELFDATA2LSB)
        return -1;

    if (eh->e_type != ET_EXEC)
        return -1;

    if (eh->e_machine != EM_386)
        return -1;

    return 0;
}

static int load_segment(struct page_directory *pd,
                        struct vnode *vn,
                        const Elf32_Phdr *ph)
{
    if (!pd || !vn || !ph)
        return -1;

    if (ph->p_memsz == 0)
        return 0;

    if (ph->p_filesz > ph->p_memsz)
        return -1;

    u32 seg_vaddr = ph->p_vaddr;
    u32 seg_offset = ph->p_offset;
    u32 seg_filesz = ph->p_filesz;
    u32 seg_memsz = ph->p_memsz;

    u32 page_size = PAGE_SIZE;
    u32 page_start = align_down(seg_vaddr, page_size);
    u32 page_end = align_up(seg_vaddr + seg_memsz, page_size);
    u32 page_count = (page_end - page_start) / page_size;

    elf_seg_page_t *pages = calloc(page_count, sizeof(*pages));
    if (!pages)
        return -1;

    /*
     * Allocate and map all pages for the segment first.
     * Then zero them via temporary mapping.
     */
    u32 i = 0;
    for (u32 addr = page_start; addr < page_end; addr += page_size, i++)
    {
        u32 phys = pmm_alloc_frame();
        if (!phys)
        {
            free(pages);
            return -1;
        }

        u32 flags = PAGE_PRESENT | PAGE_USER;
        if (ph->p_flags & PF_W)
            flags |= PAGE_RW;
        else
            flags |= PAGE_RW; /* keep writable for now; tighten later if desired */

        paging_map_page(pd, addr, phys, flags);

        pages[i].vaddr = addr;
        pages[i].phys = phys;

        void *kpage = paging_temp_map_frame(pd, phys);
        if (!kpage)
        {
            free(pages);
            return -1;
        }

        memset(kpage, 0, PAGE_SIZE);
        paging_temp_unmap_frame(pd);
    }

    /*
     * Read segment file bytes into a kernel buffer, then copy into the mapped pages.
     */
    u8 *seg_data = NULL;
    if (seg_filesz > 0)
    {
        seg_data = malloc(seg_filesz);
        if (!seg_data)
        {
            free(pages);
            return -1;
        }

        if (read_fully(vn, seg_offset, seg_data, seg_filesz) < 0)
        {
            free(seg_data);
            free(pages);
            return -1;
        }
    }

    /*
     * Copy file bytes into the mapped pages, respecting page offsets.
     */
    if (seg_filesz > 0)
    {
        u32 copied = 0;
        u32 page_off = seg_vaddr & (page_size - 1u);

        for (u32 p = 0; p < page_count && copied < seg_filesz; p++)
        {
            void *kpage = paging_temp_map_frame(pd, pages[p].phys);
            if (!kpage)
            {
                free(seg_data);
                free(pages);
                return -1;
            }

            u32 dst_off = (p == 0) ? page_off : 0;
            u32 space = page_size - dst_off;
            u32 remain = seg_filesz - copied;
            u32 chunk = (remain < space) ? remain : space;

            memcpy((u8 *)kpage + dst_off, seg_data + copied, chunk);

            copied += chunk;
            paging_temp_unmap_frame(pd);
        }
    }

    free(seg_data);
    free(pages);
    return 0;
}

int elf32_load_image(struct page_directory *pd,
                     struct vnode *vn,
                     Elf32_Addr *entry_out)
{
    if (!pd || !vn || !entry_out)
        return -1;

    KLOG_INFO("elf32_load_image: pd=%p vn=%p entry_out=%p", pd, vn, entry_out);

    Elf32_Ehdr eh;
    if (read_fully(vn, 0, &eh, sizeof(eh)) < 0)
    {
        KLOG_ERROR("elf32_load_image: failed to read ELF header from vnode %p", vn);
        return -1;
    }

    KLOG_INFO("elf32_load_image: ELF magic=%02x%02x%02x%02x class=%u data=%u type=%u machine=%u phnum=%u phentsize=%u",
              eh.e_ident[EI_MAG0], eh.e_ident[EI_MAG1], eh.e_ident[EI_MAG2], eh.e_ident[EI_MAG3],
              eh.e_ident[EI_CLASS], eh.e_ident[EI_DATA], eh.e_type, eh.e_machine,
              eh.e_phnum, eh.e_phentsize);

    if (elf32_validate(&eh) < 0)
    {
        KLOG_ERROR("elf32_load_image: ELF validation failed for vnode %p", vn);
        return -1;
    }

    if (eh.e_phentsize != sizeof(Elf32_Phdr))
    {
        return -1;
    }

    if (eh.e_phnum == 0)
    {
        return -1;
    }

    u32 ph_size = (u32)eh.e_phentsize * (u32)eh.e_phnum;
    Elf32_Phdr *phdrs = calloc(1, ph_size);
    if (!phdrs)
        return -1;

    if (read_fully(vn, eh.e_phoff, phdrs, ph_size) < 0)
    {
        free(phdrs);
        return -1;
    }

    for (u16 i = 0; i < eh.e_phnum; i++)
    {
        Elf32_Phdr *ph = &phdrs[i];

        if (ph->p_type != PT_LOAD)
            continue;

        if (load_segment(pd, vn, ph) < 0)
        {
            free(phdrs);
            return -1;
        }
    }

    *entry_out = eh.e_entry;
    free(phdrs);
    return 0;
}

static int read_from_memory(const u8 *image, u32 image_size, u32 offset, void *buf, u32 size)
{
    if (!image || !buf)
        return -1;

    if (offset + size > image_size)
        return -1;

    memcpy(buf, image + offset, size);
    return 0;
}

static int load_segment_from_memory(struct page_directory *pd,
                                    const u8 *image,
                                    u32 image_size,
                                    const Elf32_Phdr *ph)
{
    if (!pd || !image || !ph)
        return -1;

    if (ph->p_memsz == 0)
        return 0;

    if (ph->p_filesz > ph->p_memsz)
        return -1;

    u32 seg_vaddr = ph->p_vaddr;
    u32 seg_offset = ph->p_offset;
    u32 seg_filesz = ph->p_filesz;
    u32 seg_memsz = ph->p_memsz;

    u32 page_size = PAGE_SIZE;
    u32 page_start = align_down(seg_vaddr, page_size);
    u32 page_end = align_up(seg_vaddr + seg_memsz, page_size);
    u32 page_count = (page_end - page_start) / page_size;

    elf_seg_page_t *pages = calloc(page_count, sizeof(*pages));
    if (!pages)
        return -1;

    u32 i = 0;
    for (u32 addr = page_start; addr < page_end; addr += page_size, i++)
    {
        u32 phys = pmm_alloc_frame();
        if (!phys)
        {
            free(pages);
            return -1;
        }

        u32 flags = PAGE_PRESENT | PAGE_USER;
        if (ph->p_flags & PF_W)
            flags |= PAGE_RW;
        else
            flags |= PAGE_RW;

        paging_map_page(pd, addr, phys, flags);

        pages[i].vaddr = addr;
        pages[i].phys = phys;

        void *kpage = paging_temp_map_frame(pd, phys);
        if (!kpage)
        {
            free(pages);
            return -1;
        }

        memset(kpage, 0, PAGE_SIZE);
        paging_temp_unmap_frame(pd);
    }

    u8 *seg_data = NULL;
    if (seg_filesz > 0)
    {
        seg_data = malloc(seg_filesz);
        if (!seg_data)
        {
            free(pages);
            return -1;
        }

        if (read_from_memory(image, image_size, seg_offset, seg_data, seg_filesz) < 0)
        {
            free(seg_data);
            free(pages);
            return -1;
        }
    }

    if (seg_filesz > 0)
    {
        u32 copied = 0;
        u32 page_off = seg_vaddr & (page_size - 1u);

        for (u32 p = 0; p < page_count && copied < seg_filesz; p++)
        {
            void *kpage = paging_temp_map_frame(pd, pages[p].phys);
            if (!kpage)
            {
                free(seg_data);
                free(pages);
                return -1;
            }

            u32 dst_off = (p == 0) ? page_off : 0;
            u32 space = page_size - dst_off;
            u32 remain = seg_filesz - copied;
            u32 chunk = (remain < space) ? remain : space;

            memcpy((u8 *)kpage + dst_off, seg_data + copied, chunk);

            copied += chunk;
            paging_temp_unmap_frame(pd);
        }
    }

    free(seg_data);
    free(pages);
    return 0;
}

int elf32_load_image_from_memory(struct page_directory *pd,
                                 const void *image,
                                 u32 image_size,
                                 Elf32_Addr *entry_out)
{
    if (!pd || !image || !entry_out)
    {
        KLOG_ERROR("elf32_load_image_from_memory: invalid arguments");
        return -1;
    }

    KLOG_INFO("elf32_load_image_from_memory: image=%p size=%u", image, image_size);
    const u8 *bytes = image;
    if (image_size < sizeof(Elf32_Ehdr))
    {
        KLOG_ERROR("elf32_load_image_from_memory: image too small for ELF header");
        return -1;
    }

    Elf32_Ehdr eh;
    if (read_from_memory(bytes, image_size, 0, &eh, sizeof(eh)) < 0)
    {
        KLOG_ERROR("elf32_load_image_from_memory: failed to read ELF header");
        return -1;
    }

    if (elf32_validate(&eh) < 0)
    {
        KLOG_ERROR("elf32_load_image_from_memory: ELF validation failed");
        return -1;
    }

    if (eh.e_phentsize != sizeof(Elf32_Phdr) || eh.e_phnum == 0)
    {
        KLOG_ERROR("elf32_load_image_from_memory: bad program header size or count: e_phentsize=%u e_phnum=%u", eh.e_phentsize, eh.e_phnum);
        return -1;
    }

    u32 ph_size = (u32)eh.e_phentsize * (u32)eh.e_phnum;
    if (eh.e_phoff + ph_size > image_size)
        return -1;

    Elf32_Phdr *phdrs = calloc(1, ph_size);
    if (!phdrs)
        return -1;

    if (read_from_memory(bytes, image_size, eh.e_phoff, phdrs, ph_size) < 0)
    {
        free(phdrs);
        return -1;
    }

    for (u16 i = 0; i < eh.e_phnum; i++)
    {
        Elf32_Phdr *ph = &phdrs[i];

        if (ph->p_type != PT_LOAD)
            continue;

        if (load_segment_from_memory(pd, bytes, image_size, ph) < 0)
        {
            free(phdrs);
            return -1;
        }
    }

    *entry_out = eh.e_entry;
    free(phdrs);
    return 0;
}
