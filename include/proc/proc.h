#pragma once
#include "libk/types.h"
#include "sched/sched.h"
#include "fs/vfs.h"

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
void proc_set_current(process_t *p);

int proc_register_thread(Thread *t);
int proc_mark_exit(int code);

int proc_exec_vnode(struct vnode *vn);
int proc_exit(int code);
int proc_fork(void);
int proc_clone(void);
int proc_wait(int pid, int *status);
int proc_signal(int pid, int sig);