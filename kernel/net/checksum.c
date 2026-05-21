#include "net/net.h"
#include "libk/mem.h"

u16 net_checksum(const void *data, usize len)
{
    const u8 *bytes = data;
    u32 sum = 0;

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

u16 net_ipv4_checksum(const void *data, usize len)
{
    return net_checksum(data, len);
}

u16 net_pseudo_checksum(u32 src, u32 dst, u8 protocol, const void *data, usize len)
{
    u32 sum = 0;
    u8 src_bytes[4] = {
        (u8)((src >> 24) & 0xFF),
        (u8)((src >> 16) & 0xFF),
        (u8)((src >> 8) & 0xFF),
        (u8)(src & 0xFF),
    };
    u8 dst_bytes[4] = {
        (u8)((dst >> 24) & 0xFF),
        (u8)((dst >> 16) & 0xFF),
        (u8)((dst >> 8) & 0xFF),
        (u8)(dst & 0xFF),
    };

    sum += ((u16)src_bytes[0] << 8) | (u16)src_bytes[1];
    sum += ((u16)src_bytes[2] << 8) | (u16)src_bytes[3];
    sum += ((u16)dst_bytes[0] << 8) | (u16)dst_bytes[1];
    sum += ((u16)dst_bytes[2] << 8) | (u16)dst_bytes[3];

    sum += (u16)protocol;
    sum += (u16)len;

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
