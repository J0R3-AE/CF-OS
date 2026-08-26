#include "kernel/proc/process.h"

#include "kernel/sched/thread.h"

#include "kernel/mm/heap.h"
#include "kernel/mm/paging.h"

#include "libc/mem.h"
#include "libc/errno.h"
#include "libc/log.h"

#define MAX_PROCESSES 64

extern thread_t *sched_current(void);

extern struct page_directory *kernel_pd;
extern u32 kernel_pd_phys;


/* -------------------------------------------------------------------------- */
/* Process table                                                              */
/* -------------------------------------------------------------------------- */

static process_t g_processes[MAX_PROCESSES];
static u8 g_process_used[MAX_PROCESSES];

static process_t g_boot_process;

static int g_next_pid = 1;


/* -------------------------------------------------------------------------- */
/* Internal helpers                                                           */
/* -------------------------------------------------------------------------- */

static process_t *proc_alloc_slot(void)
{
    for (int i = 0; i < MAX_PROCESSES; ++i)
    {
        if (g_process_used[i])
            continue;

        g_process_used[i] = 1;

        memset(
            &g_processes[i],
            0,
            sizeof(process_t));

        return &g_processes[i];
    }

    return NULL;
}


/* -------------------------------------------------------------------------- */
/* Initialization                                                             */
/* -------------------------------------------------------------------------- */

void proc_init(void)
{
    memset(
        g_processes,
        0,
        sizeof(g_processes));

    memset(
        g_process_used,
        0,
        sizeof(g_process_used));


    memset(
        &g_boot_process,
        0,
        sizeof(g_boot_process));


    g_boot_process.pid = 0;
    g_boot_process.ppid = -1;

    g_boot_process.exit_code = 0;
    g_boot_process.alive = 1;

    g_boot_process.pd = kernel_pd;
    g_boot_process.pd_phys = kernel_pd_phys;

    g_boot_process.main_thread = NULL;


    /*
     * Reserve PID 0 conceptually for the kernel/boot process.
     */
    g_next_pid = 1;
}


/* -------------------------------------------------------------------------- */
/* Current process                                                            */
/* -------------------------------------------------------------------------- */

process_t *proc_current(void)
{
    thread_t *thread = sched_current();

    if (thread && thread->process)
        return thread->process;

    return &g_boot_process;
}


/* -------------------------------------------------------------------------- */
/* Create process                                                             */
/* -------------------------------------------------------------------------- */

process_t *proc_create(void)
{
    process_t *parent = proc_current();

    process_t *process =
        proc_alloc_slot();

    if (!process)
    {
        KLOG_ERROR("proc: process table full");
        return NULL;
    }


    process->pid = g_next_pid++;

    /*
     * For now, process creation simply inherits the kernel address
     * space. User address-space cloning will come when fork/exec
     * are implemented.
     */
    process->pd = kernel_pd;
    process->pd_phys = kernel_pd_phys;


    process->ppid =
        parent ? parent->pid : 0;

    process->exit_code = 0;
    process->alive = 1;

    process->main_thread = NULL;

    for (int i = SIG_MIN; i <= SIG_MAX; ++i)
{
    process->sigactions[i].sa_handler = SIG_DFL;
    sigemptyset(&process->sigactions[i].sa_mask);
    process->sigactions[i].sa_flags = 0;
}

    return process;
}


/* -------------------------------------------------------------------------- */
/* Attach main thread                                                         */
/* -------------------------------------------------------------------------- */

void proc_attach_thread(
    process_t *process,
    thread_t *thread)
{
    if (!process || !thread)
        return;

    process->main_thread = thread;
    thread->process = process;
}


/* -------------------------------------------------------------------------- */
/* Process exit                                                               */
/* -------------------------------------------------------------------------- */

void proc_mark_exit(int code)
{
    process_t *process =
        proc_current();

    if (!process)
        return;


    process->exit_code = code;
    process->alive = 0;


    thread_t *thread =
        sched_current();

    if (thread)
        thread->state = THREAD_ZOMBIE;
}