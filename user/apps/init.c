// user/apps/init.c

#include "../libc/syscall.h"

int init_main() {
    const char msg[] = "USERMODE PROOF: hello usermode\n";
    syscall(SYS_write, 1, (int)msg, sizeof(msg) - 1);
    return 0;
}