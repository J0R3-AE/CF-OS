#ifndef SYS_DIRENT_H
#define SYS_DIRENT_H

typedef struct {
    char name[64];
    int type;   /* optional */
} dirent_t;

#endif