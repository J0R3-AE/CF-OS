#include "net/net.h"
#include "mm/heap.h"
#include "libk/string.h"
#include "libk/log.h"

static int loopback_xmit(net_device_t *dev, const void *data, usize len)
{
    /* loopback: push straight into RX queue */
    return net_rx_push(dev, data, len);
}

void net_loopback_init(void)
{
    net_device_t *lo = calloc(1, sizeof(*lo));
    if (!lo)
    {
        klog_err("net: failed to allocate loopback device");
        return;
    }

    strcpy(lo->name, "lo");
    lo->mtu = NET_MTU_DEFAULT;
    lo->xmit = loopback_xmit;
    lo->poll_rx = NULL;

    lo->mac[0] = 0;
    lo->mac[1] = 0;
    lo->mac[2] = 0;
    lo->mac[3] = 0;
    lo->mac[4] = 0;
    lo->mac[5] = 0;

    if (net_register_device(lo) != NET_OK)
    {
        klog_err("net: failed to register loopback");
        free(lo);
        return;
    }

    klog_log("net: loopback ready");
}