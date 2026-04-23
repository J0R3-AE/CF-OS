#pragma once

#include "libk/types.h"
#include "sched/sched.h"

struct vnode;
struct page_directory;

typedef struct process
{
    int pid;
    int ppid;
    int exit_code;
    int alive;

    Thread *thread;

    struct page_directory *pd;
    u32 pd_phys;
} process_t;

void proc_init(void);
process_t *proc_current(void);
process_t *proc_create(void);
void proc_attach_thread(process_t *p, Thread *t);
void proc_mark_exit(int code);

/* keep this as a stub for now */
int proc_exec_vnode(struct vnode *vn);
int exec_elf_vnode(struct vnode *vn);