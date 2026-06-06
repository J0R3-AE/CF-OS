#include "net/net.h"
#include "libk/log.h"
#include "libk/string.h"

int net_ethernet_is_broadcast(const u8 mac[NET_MAC_LEN])
{
    if (!mac) return 0;

    for (usize i = 0; i < NET_MAC_LEN; ++i)
    {
        if (mac[i] != 0xFF) return 0;
    }

    return 1;
}

int net_ethernet_mac_equal(const u8 lhs[NET_MAC_LEN], const u8 rhs[NET_MAC_LEN])
{
    if (!lhs || !rhs) return 0;

    for (usize i = 0; i < NET_MAC_LEN; ++i)
    {
        if (lhs[i] != rhs[i]) return 0;
    }

    return 1;
}

void net_ethernet_format_mac(const u8 mac[NET_MAC_LEN], char *out, usize out_len)
{
    if (!mac || !out || out_len == 0) return;

    usize written = 0;
    
    for (usize i = 0; i < NET_MAC_LEN && written + 3 < out_len; ++i)
    {
        int value = mac[i];
        out[written++] = "0123456789ABCDEF"[(value >> 4) & 0xF];
        out[written++] = "0123456789ABCDEF"[value & 0xF];
        if (i + 1 < NET_MAC_LEN && written + 1 < out_len){out[written++] = ':';}
    }

    if (written < out_len){ out[written] = '\0';}
    else{ out[out_len - 1] = '\0';}
}

u16 net_ethernet_type(const net_ethernet_header_t *hdr)
{
    if (!hdr) return 0;

    return NET_NTOHS(hdr->ethertype);
}
