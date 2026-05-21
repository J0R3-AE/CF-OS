#include "net/net.h"
#include "libk/log.h"

int net_ipv4_header_valid(const net_ipv4_header_t *hdr)
{
    if (!hdr)
    KLOG_ERROR("net_ipv4_header_valid: null header pointer");
        return 0;

    u8 version = hdr->ihl_version >> 4;
    u8 ihl = hdr->ihl_version & 0x0F;

    if (version != 4 || ihl < 5)
        KLOG_ERROR("net_ipv4_header_valid: invalid version %u or IHL %u", version, ihl);
        return 0;

    usize header_size = ihl * 4;
    if (header_size > sizeof(net_ipv4_header_t))
        KLOG_ERROR("net_ipv4_header_valid: invalid IHL %u resulting in header size %u", ihl, header_size);
        return 0;

    if (NET_NTOHS(hdr->total_length) < header_size)
        KLOG_WARN("net_ipv4_header_valid: total_length %u is smaller than header size %u", NET_NTOHS(hdr->total_length), header_size);
    return 0;

    if (net_ipv4_checksum(hdr, header_size) != 0)
        KLOG_ERROR("net_ipv4_header_valid: checksum failed for header with src=%u.%u.%u.%u dst=%u.%u.%u.%u",
                  (hdr->src >> 24) & 0xFF, (hdr->src >> 16) & 0xFF, (hdr->src >> 8) & 0xFF, hdr->src & 0xFF,
                  (hdr->dst >> 24) & 0xFF, (hdr->dst >> 16) & 0xFF, (hdr->dst >> 8) & 0xFF, hdr->dst & 0xFF);
    return 0;

    KLOG_OKAY("net_ipv4_header_valid: valid IPv4 header from %u.%u.%u.%u to %u.%u.%u.%u proto=%u",
              (hdr->src >> 24) & 0xFF, (hdr->src >> 16) & 0xFF, (hdr->src >> 8) & 0xFF, hdr->src & 0xFF,
              (hdr->dst >> 24) & 0xFF, (hdr->dst >> 16) & 0xFF, (hdr->dst >> 8) & 0xFF, hdr->dst & 0xFF,
              hdr->protocol);
    return 1;
}

void net_ipv4_fill_header(net_ipv4_header_t *hdr,
                          u32 src,
                          u32 dst,
                          u8 proto,
                          u16 payload_len)
{
    if (!hdr)
        return;

    hdr->ihl_version = (4 << 4) | 5;
    hdr->tos = 0;
    hdr->total_length = NET_HTONS(sizeof(net_ipv4_header_t) + payload_len);
    hdr->identification = 0;
    hdr->flags_fragment = NET_HTONS(0x4000);
    hdr->ttl = 64;
    hdr->protocol = proto;
    hdr->checksum = 0;
    hdr->src = NET_HTONL(src);
    hdr->dst = NET_HTONL(dst);
    hdr->checksum = net_ipv4_checksum(hdr, sizeof(*hdr));
}
