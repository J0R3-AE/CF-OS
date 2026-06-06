// user/apps/init.c - Minimal init/installer process

#define NULL ((void *)0)

int printf(const char *fmt, ...);
int open(const char *path, int flags);
int close(int fd);
int write(int fd, const void *buf, unsigned int count);
int read(int fd, void *buf, unsigned int count);
int mkdir(const char *path);
int unlink(const char *path);

int init_main() {
    printf("===== MiniOS Installer/Init Process =====\n");
    printf("Init PID: 1\n");
    printf("\n");
    
    printf("Attempting to probe disk installation...\n");
    
    /* Try to open /install_flag on disk - if it exists, skip format */
    int fd = open("/install_flag", 0); /* O_RDONLY = 0 */
    if (fd >= 0)
    {
        printf("Disk installation already present, skipping format.\n");
        close(fd);
    }
    else
    {
        printf("No disk installation found (expected on first boot).\n");
        printf("Attempting to mark disk as installed...\n");
        
        /* Try to create installation marker on disk */
        fd = open("/install_flag", 1); /* O_WRONLY|O_CREAT = 1 */
        if (fd >= 0)
        {
            const char *marker = "OS_INSTALLED";
            write(fd, marker, 12);
            close(fd);
            printf("Created installation marker on disk.\n");
        }
        else
        {
            printf("WARNING: Could not write to disk (ramfs mode or read-only)\n");
            printf("This is expected on first-time live boot.\n");
        }
    }
    
    printf("\n");
    printf("Boot sequence complete.\n");
    printf("System ready for user interaction.\n");
    printf("\n");
    
    return 0;
}