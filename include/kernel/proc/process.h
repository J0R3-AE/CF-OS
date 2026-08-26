#pragma once

#include "libc/types.h"
#include "kernel/signal/signal.h"

struct thread;
struct page_directory;

typedef struct process
{
    int pid;
    int ppid;

    int exit_code;
    int alive;

    struct page_directory *pd;
    u32 pd_phys;

    struct thread *main_thread;

    struct sigaction sigactions[SIG_MAX + 1];

} process_t;


/* Lifecycle */
void proc_init(void);

process_t *proc_current(void);
process_t *proc_create(void);

void proc_attach_thread(
    process_t *process,
    struct thread *thread);

void proc_mark_exit(int code);