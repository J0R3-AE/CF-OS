#include "net/net.h"
#include "libk/log.h"

int e1000_probe(void)
{
    KLOG_INFO("e1000: probe stub called");
    return NET_ENOENT;
}

int e1000_init(net_device_t *dev)
{
    if (!dev) return NET_EINVAL;

    KLOG_INFO("e1000: init stub for device %s", dev->name);
    return NET_ENOENT;
}
