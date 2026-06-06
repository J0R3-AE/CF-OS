#include "net/net.h"
#include "mm/heap.h"
#include "libk/log.h"

static int loopback_xmit(net_device_t *dev, const void *data, usize len)
{return net_rx_push(dev, data, len);}

static int loopback_poll(net_device_t *dev, void *buf, usize len, usize *out)
{return net_rx_pop(dev, buf, len, out);}

void net_loopback_init(void)
{
    u8 mac[NET_MAC_LEN] = {0, 0, 0, 0, 0, 0};
    net_device_t *lo = net_device_alloc("lo", mac, NET_MTU_DEFAULT, loopback_xmit, loopback_poll, NULL);
    
    if (!lo) return;

    if (net_register_device(lo) != NET_OK)
    {
        net_device_free(lo);
        return;
    }

    KLOG_INFO("loopback interface 'lo' initialized");
}
