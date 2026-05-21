#include "net/net.h"

int net_nic_initialize(net_device_t *dev)
{
    if (!dev)
        return NET_EINVAL;

    if (!dev->xmit)
        return NET_ENOENT;

    if (dev->mtu == 0)
        dev->mtu = NET_MTU_DEFAULT;

    if (net_register_device(dev) != NET_OK)
        return NET_EBUSY;

    return NET_OK;
}
