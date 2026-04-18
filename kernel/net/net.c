#include "net/net.h"

#include "mm/heap.h"
#include "libk/string.h"
#include "libk/log.h"

static Link g_net_devices;
static int g_net_ready = 0;

static void net_copy_name(char *dst, const char *src)
{
    usize i = 0;
    if (!dst)
        return;

    if (!src)
    {
        dst[0] = '\0';
        return;
    }

    while (src[i] && i < NET_NAME_MAX - 1)
    {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static int net_queue_empty(net_device_t *dev)
{
    return !dev || ListIsEmpty(&dev->queue);
}

void net_init(void)
{
    if (g_net_ready)
        return;

    ListInit(&g_net_devices);
    g_net_ready = 1;
    klog_info("net: initialized");
}

net_packet_t *net_packet_alloc(usize len)
{
    net_packet_t *pkt = calloc(1, sizeof(*pkt));
    if (!pkt)
        return NULL;

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
    if (!pkt)
        return;

    free(pkt->data);
    free(pkt);
}

int net_register_device(net_device_t *dev)
{
    if (!dev || !dev->name[0])
        return NET_EINVAL;

    if (!g_net_ready)
        net_init();

    for (Link *it = g_net_devices.next; it != &g_net_devices; it = it->next)
    {
        net_device_t *cur = LinkData(it, net_device_t, rx_queue_link);
        if (strcmp(cur->name, dev->name) == 0)
            return NET_EBUSY;
    }

    if (dev->mtu == 0)
        dev->mtu = NET_MTU_DEFAULT;

    ListInit(&dev->queue);
    ListBefore(&g_net_devices, &dev->rx_queue_link);

    klog_info("net: registered device %s", dev->name);
    return NET_OK;
}

int net_unregister_device(net_device_t *dev)
{
    if (!dev)
        return NET_EINVAL;

    if (dev->rx_queue_link.next && dev->rx_queue_link.prev)
        ListRemove(&dev->rx_queue_link);

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
    if (!name)
        return NULL;

    for (Link *it = g_net_devices.next; it != &g_net_devices; it = it->next)
    {
        net_device_t *dev = LinkData(it, net_device_t, rx_queue_link);
        if (strcmp(dev->name, name) == 0)
            return dev;
    }

    return NULL;
}

int net_send(net_device_t *dev, const void *data, usize len)
{
    if (!dev || !data || len == 0)
        klog_err("net: invalid arguments to net_send");
    return NET_EINVAL;

    if (!dev->xmit)
        klog_err("net: device %s has no xmit function", dev->name);
    return NET_ENOENT;

    if (len > dev->mtu)
        klog_debug("net: packet size %zu exceeds MTU %zu for device %s", len, dev->mtu, dev->name);
    return NET_E2BIG;

    return dev->xmit(dev, data, len);
}

int net_rx_push(net_device_t *dev, const void *data, usize len)
{
    if (!dev || !data || len == 0)
        klog_err("net: invalid arguments to net_rx_push");
    return NET_EINVAL;

    net_packet_t *pkt = net_packet_alloc(len);
    if (!pkt)
        klog_err("net: failed to allocate packet for RX push");
    return NET_ENOMEM;

    memcpy(pkt->data, data, len);
    pkt->len = len;

    ListBefore(&dev->queue, &pkt->link);
    klog_debug("net: pushed packet into %s queue (len=%zu)", dev->name, len);
    return NET_OK;
}

int net_rx_pop(net_device_t *dev, void *buf, usize len, usize *out)
{
    if (!dev || !buf)
        klog_err("net: invalid arguments to net_rx_pop");
    return NET_EINVAL;

    if (net_queue_empty(dev))
        klog_err("net: no packets to pop from %s", dev->name);
    return NET_EAGAIN;

    Link *node = dev->queue.next;
    ListRemove(node);

    net_packet_t *pkt = LinkData(node, net_packet_t, link);
    usize n = pkt->len;

    if (n > len)
        n = len;

    memcpy(buf, pkt->data, n);

    if (out)
        *out = n;

    net_packet_free(pkt);
    klog_debug("net: popped packet from %s", dev->name);
    return NET_OK;
}

void net_dump_device(net_device_t *dev)
{
    if (!dev)
    {
        klog_info("net: <null device>");
        return;
    }

    klog_info("net: dev=%s mtu=%u flags=%x", dev->name, (u32)dev->mtu, dev->flags);
    klog_info("net: mac=%02x:%02x:%02x:%02x:%02x:%02x",
              dev->mac[0], dev->mac[1], dev->mac[2],
              dev->mac[3], dev->mac[4], dev->mac[5]);
}