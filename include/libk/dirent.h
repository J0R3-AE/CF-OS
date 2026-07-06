#ifndef SYS_DIRENT_H
#define SYS_DIRENT_H

typedef struct {
    unsigned short lenght;
    unsigned char type;   /* optional */
    char name[256];
} dirent_t;

#endif