#include "net/net.h"

u16 net_udp_checksum(const u8 src_ip[4], const u8 dst_ip[4], const void *data, usize len)
{
    if (!src_ip || !dst_ip || !data)
        return 0;

    u32 sum = 0;
    for (usize i = 0; i < 4; i += 2)
        sum += ((u16)src_ip[i] << 8) | (u16)src_ip[i + 1];

    for (usize i = 0; i < 4; i += 2)
        sum += ((u16)dst_ip[i] << 8) | (u16)dst_ip[i + 1];

    sum += NET_HTONS(NET_IPPROTO_UDP);
    sum += NET_HTONS((u16)len);

    const u8 *bytes = data;
    while (len > 1)
    {
        sum += ((u16)bytes[0] << 8) | (u16)bytes[1];
        bytes += 2;
        len -= 2;
    }

    if (len == 1)
        sum += (u16)bytes[0] << 8;

    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

    return (u16)~sum;
}
