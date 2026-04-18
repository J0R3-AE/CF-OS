#pragma once

#include "libk/types.h"
#include "libk/link.h"

#define IPC_NAME_MAX 32
#define IPC_MAX_DATA 256

#define IPC_OK     0
#define IPC_EINVAL -1
#define IPC_ENOMEM -2
#define IPC_EAGAIN -3
#define IPC_E2BIG  -4
#define IPC_ENOENT -5
#define IPC_EEXIST -6

typedef struct ipc_msg
{
    u32 id;
    u32 src;
    u32 type;
    u32 flags;
    usize len;
    u8 data[IPC_MAX_DATA];

    Link link;
} ipc_msg_t;

typedef enum {
    IPC_MSG_NONE = 0,
    IPC_MSG_NET_PACKET,
    IPC_MSG_SIGNAL,
    IPC_MSG_DATA,
} ipc_msg_type_t;

typedef struct ipc_port
{
    u32 id;
    char name[IPC_NAME_MAX];
    void *owner;   /* future use: thread/task that owns this port */
    usize pending; /* queued messages */
    Link inbox;
} ipc_port_t;

void ipc_init(void);

ipc_port_t *ipc_port_create(void);
void ipc_port_destroy(ipc_port_t *port);

int ipc_send(ipc_port_t *dst, u32 src, u32 type, const void *data, usize len);
int ipc_recv(ipc_port_t *port, ipc_msg_t *out);

usize ipc_pending(ipc_port_t *port);

int ipc_register_service(const char *name, ipc_port_t *port);
int ipc_unregister_service(const char *name);
int ipc_unregister_port(ipc_port_t *port);
ipc_port_t *ipc_lookup_service(const char *name);