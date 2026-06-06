#include "net/net.h"
#include "libk/log.h"
#include "libk/mem.h"

#define NET_ARP_CACHE_SIZE 16

typedef struct net_arp_entry
{
    u8 ip[4];
    u8 mac[NET_MAC_LEN];
    b8 valid;
} net_arp_entry_t;

static net_arp_entry_t g_arp_cache[NET_ARP_CACHE_SIZE];

int net_arp_init(void)
{
    for (usize i = 0; i < NET_ARP_CACHE_SIZE; ++i){g_arp_cache[i].valid = false;}

    return NET_OK;
}

int net_arp_insert(const u8 ip[4], const u8 mac[NET_MAC_LEN])
{
    if (!ip || !mac) return NET_EINVAL;

    for (usize i = 0; i < NET_ARP_CACHE_SIZE; ++i)
    {
        if (g_arp_cache[i].valid && memcmp(g_arp_cache[i].ip, ip, sizeof(g_arp_cache[i].ip)) == 0)
        {
            memcpy(g_arp_cache[i].mac, mac, NET_MAC_LEN);
            return NET_OK;
        }
    }

    for (usize i = 0; i < NET_ARP_CACHE_SIZE; ++i)
    {
        if (!g_arp_cache[i].valid)
        {
            g_arp_cache[i].valid = true;
            memcpy(g_arp_cache[i].ip, ip, sizeof(g_arp_cache[i].ip));
            memcpy(g_arp_cache[i].mac, mac, NET_MAC_LEN);

            return NET_OK;
        }
    }

    g_arp_cache[0].valid = true;
    memcpy(g_arp_cache[0].ip, ip, sizeof(g_arp_cache[0].ip));
    memcpy(g_arp_cache[0].mac, mac, NET_MAC_LEN);

    return NET_OK;
}

int net_arp_lookup(const u8 ip[4], u8 mac_out[NET_MAC_LEN])
{
    if (!ip || !mac_out) return NET_EINVAL;

    for (usize i = 0; i < NET_ARP_CACHE_SIZE; ++i)
    {
        if (g_arp_cache[i].valid && memcmp(g_arp_cache[i].ip, ip, sizeof(g_arp_cache[i].ip)) == 0)
        {
            memcpy(mac_out, g_arp_cache[i].mac, NET_MAC_LEN);
            return NET_OK;
        }
    }

    return NET_ENOENT;
}

int net_arp_process(net_device_t *dev, const void *frame, usize len)
{
    if (!dev || !frame || len < sizeof(net_ethernet_header_t) + sizeof(net_arp_header_t)) return NET_EINVAL;

    const net_ethernet_header_t *eth = frame;
    
    if (net_ethernet_type(eth) != NET_ETHERTYPE_ARP) return NET_EINVAL;

    const net_arp_header_t *arp = (const net_arp_header_t *)((const u8 *)frame + sizeof(net_ethernet_header_t));
    u16 op = NET_NTOHS(arp->op);

    if (op == NET_ARP_OPCODE_REPLY)
    {
        net_arp_insert(arp->spa, arp->sha);
        return NET_OK;
    }

    if (op == NET_ARP_OPCODE_REQUEST)
    {
        net_arp_insert(arp->spa, arp->sha);
        return NET_OK;
    }

    return NET_OK;
}
