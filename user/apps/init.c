// user/apps/init.c

int printf(const char *fmt, ...);

int init_main() {
    printf("USERMODE INIT: hello from init process\n");
    printf("Init PID: %d\n", 1);  // TODO: call getpid syscall
    
    // Keep running
    while (1) {
        // TODO: wait for children, handle signals
    }
    
    return 0;
}