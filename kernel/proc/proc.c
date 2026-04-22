#include "proc/proc.h"
#include "libk/log.h"
#include "libk/mem.h"
#include "libk/errno.h"
#include "sched/sched.h"

#define MAX_PROCS 64


static process_t g_procs[MAX_PROCS];
static process_t *g_curproc = NULL;
static int g_next_pid = 1;

static process_t *proc_alloc(void)
{
    for (int i = 0; i < MAX_PROCS; i++)
    {
        if (!g_procs[i].alive)
        {
            memset(&g_procs[i], 0, sizeof(g_procs[i]));
            g_procs[i].alive = 1;
            g_procs[i].pid = g_next_pid++;
            return &g_procs[i];
        }
    }
    return NULL;
}

void proc_init(void)
{
    memset(g_procs, 0, sizeof(g_procs));
    g_next_pid = 1;
    g_curproc = proc_alloc();
    if (g_curproc)
        g_curproc->ppid = 0;
    klog_info("proc: initialized");
}

process_t *proc_current(void)
{
    return g_curproc;
}

void proc_set_current(process_t *p)
{
    g_curproc = p;
}

int proc_register_thread(Thread *t)
{
    if (!t)
        return -EINVAL;

    process_t *p = proc_alloc();
    if (!p)
        return -ENOMEM;

    p->thread = t;
    p->ppid = g_curproc ? g_curproc->pid : 0;
    g_curproc = p;
    return p->pid;
}

int proc_mark_exit(int code)
{
    if (!g_curproc)
        return -EINVAL;

    g_curproc->exit_code = code;
    g_curproc->alive = 0;
    return 0;
}

process_t *proc_create(void)
{
    process_t *p = malloc(sizeof(process_t));
    if (!p)
        return NULL;

    memset(p, 0, sizeof(process_t));

    p->pid = g_next_pid++;
    p->alive = 1;

    p->pd = paging_create_user_pd(&p->pd_phys);
    if (!p->pd)
        return NULL;

    return p;
}