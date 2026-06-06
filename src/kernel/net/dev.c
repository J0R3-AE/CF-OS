#include "net/net.h"
#include "mm/heap.h"
#include "libk/string.h"
#include "libk/log.h"

static void net_copy_name(char *dst, const char *src)
{
    usize index = 0;
    if (!dst)
        return;

    if (!src)
    {
        dst[0] = '\0';
        return;
    }

    while (src[index] && index < NET_NAME_MAX - 1)
    {
        dst[index] = src[index];
        index++;
    }

    dst[index] = '\0';
}

net_device_t *net_device_alloc(const char *name,
                              const u8 mac[NET_MAC_LEN],
                              usize mtu,
                              net_xmit_fn xmit,
                              net_poll_fn poll_rx,
                              void *priv)
{
    if (!name || !xmit)
        return NULL;

    net_device_t *dev = calloc(1, sizeof(*dev));
    if (!dev)
        return NULL;

    net_copy_name(dev->name, name);

    if (mac)
        memcpy(dev->mac, mac, NET_MAC_LEN);
    else
        memset(dev->mac, 0, NET_MAC_LEN);

    dev->mtu = mtu == 0 ? NET_MTU_DEFAULT : mtu;
    dev->xmit = xmit;
    dev->poll_rx = poll_rx;
    dev->priv = priv;

    ListInit(&dev->queue);
    ListInit(&dev->rx_queue_link);

    return dev;
}

void net_device_free(net_device_t *dev)
{
    if (!dev)
        return;

    net_unregister_device(dev);
    free(dev);
}

int net_device_poll(net_device_t *dev, void *buf, usize len, usize *out)
{
    if (!dev)
        return NET_EINVAL;

    if (dev->poll_rx)
        return dev->poll_rx(dev, buf, len, out);

    return net_rx_pop(dev, buf, len, out);
}
