#pragma once

#include "libk/types.h"
#include "libk/link.h"

#define NET_NAME_MAX 32
#define NET_MAC_LEN 6
#define NET_MTU_MIN 576
#define NET_MTU_DEFAULT 1500
#define NET_MTU_MAX 9000

#define NET_OK 0
#define NET_EINVAL -1
#define NET_ENOMEM -2
#define NET_ENOENT -3
#define NET_EBUSY -4
#define NET_EAGAIN -5
#define NET_E2BIG -6

typedef struct net_packet
{
    usize len;
    usize capacity;
    u8 *data;

    u32 proto; /* future use */
    u32 flags; /* future use */
    Link link;
} net_packet_t;

typedef struct net_device net_device_t;

typedef int (*net_xmit_fn)(net_device_t *dev, const void *data, usize len);
typedef int (*net_poll_fn)(net_device_t *dev, void *buf, usize len, usize *out);

struct net_device
{
    char name[NET_NAME_MAX];
    u8 mac[NET_MAC_LEN];

    usize mtu;
    u32 flags;
    void *priv;

    net_xmit_fn xmit;
    net_poll_fn poll_rx;

    Link rx_queue_link; /* for device list */
    Link queue;         /* queue of net_packet_t */
};

void net_init(void);

/* device registration */
int net_register_device(net_device_t *dev);
int net_unregister_device(net_device_t *dev);
net_device_t *net_find_device(const char *name);

/* packet helpers */
net_packet_t *net_packet_alloc(usize len);
void net_packet_free(net_packet_t *pkt);

/* send / receive */
int net_send(net_device_t *dev, const void *data, usize len);
int net_rx_push(net_device_t *dev, const void *data, usize len);
int net_rx_pop(net_device_t *dev, void *buf, usize len, usize *out);

/* debugging */
void net_dump_device(net_device_t *dev);