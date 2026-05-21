#include "net/net.h"
#include "mm/heap.h"
#include "libk/mem.h"

net_packet_t *net_packet_clone(const net_packet_t *pkt)
{
    if (!pkt)
        return NULL;

    net_packet_t *copy = net_packet_alloc(pkt->len);
    if (!copy)
        return NULL;

    memcpy(copy->data, pkt->data, pkt->len);
    copy->len = pkt->len;
    copy->capacity = pkt->capacity;

    return copy;
}
