#include "net/net.h"

#include "mm/heap.h"
#include "libk/string.h"
#include "libk/mem.h"
#include "libk/log.h"

static Link g_net_devices;
static int g_net_ready = 0;

static int net_queue_empty(net_device_t *dev)
{return !dev || ListIsEmpty(&dev->queue);}

void net_init(void)
{
    if (g_net_ready) return;

    ListInit(&g_net_devices);
    net_arp_init();
    net_socket_init();
    g_net_ready = 1;
}

net_packet_t *net_packet_alloc(usize len)
{
    net_packet_t *pkt = calloc(1, sizeof(*pkt));

    if (!pkt) return NULL;

    if (len > 0)
    {
        pkt->data = calloc(1, len);
        if (!pkt->data)
        {
            free(pkt);
            return NULL;
        }
    }

    pkt->len = len;
    pkt->capacity = len;
    ListInit(&pkt->link);

    return pkt;
}

void net_packet_free(net_packet_t *pkt)
{
    if (!pkt) return;

    free(pkt->data);
    free(pkt);
}

int net_register_device(net_device_t *dev)
{
    if (!dev || !dev->name[0]) return NET_EINVAL;

    if (!g_net_ready){net_init();}

    for (Link *it = g_net_devices.next; it != &g_net_devices; it = it->next)
    {
        net_device_t *cur = LinkData(it, net_device_t, rx_queue_link);
        if (strcmp(cur->name, dev->name) == 0) return NET_EBUSY;
    }

    if (dev->mtu == 0){dev->mtu = NET_MTU_DEFAULT;}

    ListInit(&dev->queue);
    ListInit(&dev->rx_queue_link);
    ListBefore(&g_net_devices, &dev->rx_queue_link);

    return NET_OK;
}

int net_unregister_device(net_device_t *dev)
{
    if (!dev) return NET_EINVAL;

    if (dev->rx_queue_link.next && dev->rx_queue_link.prev){ListRemove(&dev->rx_queue_link);}

    while (!ListIsEmpty(&dev->queue))
    {
        Link *node = dev->queue.next;
        ListRemove(node);
        net_packet_t *pkt = LinkData(node, net_packet_t, link);
        net_packet_free(pkt);
    }

    return NET_OK;
}

net_device_t *net_find_device(const char *name)
{
    if (!name)return NULL;

    for (Link *it = g_net_devices.next; it != &g_net_devices; it = it->next)
    {
        net_device_t *dev = LinkData(it, net_device_t, rx_queue_link);
        if (strcmp(dev->name, name) == 0) return dev;
    }

    return NULL;
}

int net_send(net_device_t *dev, const void *data, usize len)
{
    if (!dev || !data || len == 0) return NET_EINVAL;

    if (!dev->xmit) return NET_ENOENT;

    if (len > dev->mtu) return NET_E2BIG;

    return dev->xmit(dev, data, len);
}

int net_rx_push(net_device_t *dev, const void *data, usize len)
{
    if (!dev || !data || len == 0) return NET_EINVAL;

    net_packet_t *pkt = net_packet_alloc(len);

    if (!pkt) return NET_ENOMEM;

    memcpy(pkt->data, data, len);
    pkt->len = len;

    ListBefore(&dev->queue, &pkt->link);
    return NET_OK;
}

int net_rx_pop(net_device_t *dev, void *buf, usize len, usize *out)
{
    if (!dev || !buf || len == 0) return NET_EINVAL;
    if (net_queue_empty(dev)) return NET_EAGAIN;

    Link *node = dev->queue.next;

    ListRemove(node);

    net_packet_t *pkt = LinkData(node, net_packet_t, link);
    usize n = pkt->len;

    if (n > len){n = len;}

    memcpy(buf, pkt->data, n);

    if (out){*out = n;}

    net_packet_free(pkt);

    return NET_OK;
}

void net_dump_device(net_device_t *dev)
{
    if (!dev) return;

    usize count = 0;
    for (Link *it = dev->queue.next; it != &dev->queue; it = it->next){count++;}

    KLOG_INFO("net device %s: mac=%x:%x:%x:%x:%x:%x mtu=%u flags=0x%x queue=%u",
              dev->name,
              dev->mac[0], dev->mac[1], dev->mac[2],
              dev->mac[3], dev->mac[4], dev->mac[5],
              (unsigned)dev->mtu,
              (unsigned)dev->flags,
              (unsigned)count);
}
