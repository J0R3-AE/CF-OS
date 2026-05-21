#include "proc/proc.h"
#include "mm/heap.h"
#include "mm/paging.h"
#include "libk/mem.h"
#include "libk/errno.h"
#include "libk/log.h"

extern Thread *g_current;
extern struct page_directory *kernel_pd;
extern u32 kernel_pd_phys;

#define MAX_PROCS 64

static process_t g_boot_proc;
static process_t g_procs[MAX_PROCS];
static u8 g_proc_used[MAX_PROCS];
static int g_next_pid = 1;
static int g_boot_ready = 0;

static process_t *proc_alloc_slot(void)
{
    for (int i = 0; i < MAX_PROCS; i++)
    {
        if (!g_proc_used[i])
        {
            g_proc_used[i] = 1;
            memset(&g_procs[i], 0, sizeof(g_procs[i]));
            return &g_procs[i];
        }
    }
    return NULL;
}

void proc_init(void)
{
    memset(&g_boot_proc, 0, sizeof(g_boot_proc));
    g_boot_proc.pid = 0;
    g_boot_proc.ppid = -1;
    g_boot_proc.alive = 1;
    g_boot_proc.pd = kernel_pd;
    g_boot_proc.pd_phys = kernel_pd_phys;
    g_boot_proc.thread = NULL;

    memset(g_procs, 0, sizeof(g_procs));
    memset(g_proc_used, 0, sizeof(g_proc_used));
    g_next_pid = 1;
    g_boot_ready = 1;
}

process_t *proc_current(void)
{
    if (g_current && g_current->proc)
        return g_current->proc;

    return &g_boot_proc;
}

process_t *proc_create(void)
{
    process_t *p = proc_alloc_slot();
    if (!p)
        return NULL;

    p->pid = g_next_pid++;
    p->ppid = proc_current() ? proc_current()->pid : 0;
    p->exit_code = 0;
    p->alive = 1;
    p->thread = NULL;
    p->pd = kernel_pd;
    p->pd_phys = kernel_pd_phys;

    return p;
}

void proc_attach_thread(process_t *p, Thread *t)
{
    if (!p || !t)
        return;

    p->thread = t;
    t->proc = p;
}

void proc_mark_exit(int code)
{
    process_t *p = proc_current();
    if (!p)
        return;

    p->exit_code = code;
    p->alive = 0;

    if (g_current)
        g_current->state = THREAD_ZOMBIE;
}