#include "ipc/ipc.h"

#include "mm/heap.h"
#include "libk/string.h"
#include "libk/mem.h"
#include "libk/log.h"

typedef struct ipc_service_node
{
    Link link;
    char name[IPC_NAME_MAX];
    ipc_port_t *port;
} ipc_service_node_t;

static Link g_service_registry;
static u32 g_next_port_id = 1;
static u32 g_next_msg_id = 1;
static int g_ipc_ready = 0;

static void ipc_copy_name(char *dst, const char *src)
{
    usize i = 0;
    if (!dst) return;

    if (!src){dst[0] = '\0'; return;}

    while (src[i] && i < (IPC_NAME_MAX - 1)){dst[i] = src[i]; i++;}

    dst[i] = '\0';
}

static ipc_service_node_t *ipc_find_service(const char *name)
{
    if (!name) return NULL;

    for (Link *it = g_service_registry.next; it != &g_service_registry; it = it->next)
    {
        ipc_service_node_t *svc = LinkData(it, ipc_service_node_t, link);
        if (strcmp(svc->name, name) == 0) return svc;
    }

    return NULL;
}

static void ipc_free_queue(ipc_port_t *port)
{
    if (!port)  return;

    while (!ListIsEmpty(&port->inbox))
    {
        Link *node = port->inbox.next;
        ListRemove(node);

        ipc_msg_t *msg = LinkData(node, ipc_msg_t, link);
        free(msg);
    }

    port->pending = 0;
}

void ipc_init(void)
{
    if (g_ipc_ready) return;

    ListInit(&g_service_registry);
    g_next_port_id = 1;
    g_next_msg_id = 1;
    g_ipc_ready = 1;
}

ipc_port_t *ipc_port_create(void)
{
    ipc_port_t *port = calloc(1, sizeof(*port));

    if (!port)  return NULL;

    port->id = g_next_port_id++;
    port->owner = NULL;
    port->pending = 0;
    port->name[0] = '\0';
    ListInit(&port->inbox);

    return port;
}

void ipc_port_destroy(ipc_port_t *port)
{
    if (!port) return;

    ipc_unregister_port(port);
    ipc_free_queue(port);
    free(port);
}

int ipc_send(ipc_port_t *dst, u32 src, u32 type, const void *data, usize len)
{
    if (!dst || (!data && len != 0))  return IPC_EINVAL;

    if (len > IPC_MAX_DATA) return IPC_E2BIG;

    ipc_msg_t *msg = calloc(1, sizeof(*msg));

    if (!msg)  return IPC_ENOMEM;

    msg->id = g_next_msg_id++;
    msg->src = src;
    msg->type = type;
    msg->flags = 0;
    msg->len = len;

    if (len > 0){memcpy(msg->data, data, len);}

    ListBefore(&dst->inbox, &msg->link);

    dst->pending++;

    return IPC_OK;
}

int ipc_recv(ipc_port_t *port, ipc_msg_t *out)
{
    if (!port || !out) return IPC_EINVAL;

    if (ListIsEmpty(&port->inbox)) return IPC_EAGAIN;

    Link *node = port->inbox.next;

    ListRemove(node);

    ipc_msg_t *msg = LinkData(node, ipc_msg_t, link);

    port->pending--;

    out->id = msg->id;
    out->src = msg->src;
    out->type = msg->type;
    out->flags = msg->flags;
    out->len = msg->len;

    if (msg->len > 0) memcpy(out->data, msg->data, msg->len);

    free(msg);

    return IPC_OK;
}

usize ipc_pending(ipc_port_t *port)
{
    if (!port) return 0;

    return port->pending;
}

int ipc_register_service(const char *name, ipc_port_t *port)
{
    if (!name || !port) return IPC_EINVAL;

    if (ipc_find_service(name)) return IPC_EEXIST;

    ipc_service_node_t *svc = calloc(1, sizeof(*svc));

    if (!svc) return IPC_ENOMEM;

    ipc_copy_name(svc->name, name);
    svc->port = port;
    ipc_copy_name(port->name, name);

    ListBefore(&g_service_registry, &svc->link);

    return IPC_OK;
}

int ipc_unregister_service(const char *name)
{
    ipc_service_node_t *svc = ipc_find_service(name);

    if (!svc) return IPC_ENOENT;

    ListRemove(&svc->link);

    free(svc);

    return IPC_OK;
}

int ipc_unregister_port(ipc_port_t *port)
{
    if (!port) return IPC_EINVAL;

    for (Link *it = g_service_registry.next; it != &g_service_registry;)
    {
        Link *next = it->next;
        ipc_service_node_t *svc = LinkData(it, ipc_service_node_t, link);

        if (svc->port == port)
        {
            ListRemove(&svc->link);
            free(svc);
        }

        it = next;
    }

    return IPC_OK;
}

ipc_port_t *ipc_lookup_service(const char *name)
{
    ipc_service_node_t *svc = ipc_find_service(name);
    
    if (!svc) return NULL;

    return svc->port;
}