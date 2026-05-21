#pragma once

#include "libk/types.h"
#include "libk/link.h"

#define NET_NAME_MAX 32
#define NET_MAC_LEN 6
#define NET_MTU_MIN 576
#define NET_MTU_DEFAULT 1500
#define NET_MTU_MAX 9000

#define NET_OK 0
#define NET_EINVAL -1
#define NET_ENOMEM -2
#define NET_ENOENT -3
#define NET_EBUSY -4
#define NET_EAGAIN -5
#define NET_E2BIG -6

#define NET_IPPROTO_ICMP 1
#define NET_IPPROTO_TCP 6
#define NET_IPPROTO_UDP 17

#define NET_ETHERTYPE_IPV4 0x0800
#define NET_ETHERTYPE_ARP  0x0806

#define NET_HW_TYPE_ETHERNET 1

#define NET_ARP_OPCODE_REQUEST 1
#define NET_ARP_OPCODE_REPLY   2

#define NET_HTONS(x) ((u16)((((u16)(x) << 8) & 0xff00) | (((u16)(x) >> 8) & 0x00ff)))
#define NET_NTOHS(x) NET_HTONS(x)
#define NET_HTONL(x) (((u32)NET_HTONS((u32)(x) & 0xffff) << 16) | NET_HTONS(((u32)(x) >> 16) & 0xffff))
#define NET_NTOHL(x) NET_HTONL(x)

typedef struct net_ethernet_header
{
    u8 dst[NET_MAC_LEN];
    u8 src[NET_MAC_LEN];
    u16 ethertype;
} PACKED net_ethernet_header_t;

typedef struct net_arp_header
{
    u16 hw_type;
    u16 proto_type;
    u8 hw_len;
    u8 proto_len;
    u16 op;
    u8 sha[NET_MAC_LEN];
    u8 spa[4];
    u8 tha[NET_MAC_LEN];
    u8 tpa[4];
} PACKED net_arp_header_t;

typedef struct net_ipv4_header
{
    u8 ihl_version;
    u8 tos;
    u16 total_length;
    u16 identification;
    u16 flags_fragment;
    u8 ttl;
    u8 protocol;
    u16 checksum;
    u32 src;
    u32 dst;
} PACKED net_ipv4_header_t;

typedef struct net_udp_header
{
    u16 src_port;
    u16 dst_port;
    u16 length;
    u16 checksum;
} PACKED net_udp_header_t;

typedef struct net_tcp_header
{
    u16 src_port;
    u16 dst_port;
    u32 seq_num;
    u32 ack_num;
    u16 offset_flags;
    u16 window;
    u16 checksum;
    u16 urgent;
} PACKED net_tcp_header_t;

typedef struct net_packet
{
    usize len;
    usize capacity;
    u8 *data;

    u32 proto;
    u32 flags;
    Link link;
} net_packet_t;

typedef struct net_device net_device_t;

typedef int (*net_xmit_fn)(net_device_t *dev, const void *data, usize len);
typedef int (*net_poll_fn)(net_device_t *dev, void *buf, usize len, usize *out);

struct net_device
{
    char name[NET_NAME_MAX];
    u8 mac[NET_MAC_LEN];

    usize mtu;
    u32 flags;
    void *priv;

    net_xmit_fn xmit;
    net_poll_fn poll_rx;

    Link rx_queue_link;
    Link queue;
};

typedef enum {
    NET_SOCK_STREAM = 1,
    NET_SOCK_DGRAM  = 2,
} net_socket_type_t;

typedef struct net_socket
{
    int fd;
    net_socket_type_t type;
    u16 port;
    net_device_t *device;
    Link link;
    Link rx_queue;
} net_socket_t;

void net_init(void);

int net_register_device(net_device_t *dev);
int net_unregister_device(net_device_t *dev);
net_device_t *net_find_device(const char *name);

net_device_t *net_device_alloc(const char *name,
                              const u8 mac[NET_MAC_LEN],
                              usize mtu,
                              net_xmit_fn xmit,
                              net_poll_fn poll_rx,
                              void *priv);
void net_device_free(net_device_t *dev);
int net_device_poll(net_device_t *dev, void *buf, usize len, usize *out);

net_packet_t *net_packet_alloc(usize len);
net_packet_t *net_packet_clone(const net_packet_t *pkt);
void net_packet_free(net_packet_t *pkt);

int net_send(net_device_t *dev, const void *data, usize len);
int net_rx_push(net_device_t *dev, const void *data, usize len);
int net_rx_pop(net_device_t *dev, void *buf, usize len, usize *out);

u16 net_checksum(const void *data, usize len);
u16 net_ipv4_checksum(const void *data, usize len);
u16 net_pseudo_checksum(u32 src, u32 dst, u8 protocol, const void *data, usize len);

int net_arp_init(void);
int net_arp_insert(const u8 ip[4], const u8 mac[NET_MAC_LEN]);
int net_arp_lookup(const u8 ip[4], u8 mac_out[NET_MAC_LEN]);
int net_arp_process(net_device_t *dev, const void *frame, usize len);

u16 net_udp_checksum(const u8 src_ip[4], const u8 dst_ip[4], const void *data, usize len);
u16 net_tcp_checksum(const u8 src_ip[4], const u8 dst_ip[4], const void *data, usize len);

int net_ethernet_is_broadcast(const u8 mac[NET_MAC_LEN]);
int net_ethernet_mac_equal(const u8 lhs[NET_MAC_LEN], const u8 rhs[NET_MAC_LEN]);
void net_ethernet_format_mac(const u8 mac[NET_MAC_LEN], char *out, usize out_len);
u16 net_ethernet_type(const net_ethernet_header_t *hdr);

int net_socket_init(void);
net_socket_t *net_socket_create(net_socket_type_t type);
int net_socket_bind(net_socket_t *sock, u16 port);
int net_socket_close(net_socket_t *sock);
int net_socket_sendto(net_socket_t *sock,
                      const u8 dst_ip[4],
                      u16 dst_port,
                      const void *data,
                      usize len);
int net_socket_recvfrom(net_socket_t *sock,
                        void *buf,
                        usize len,
                        usize *out,
                        u8 src_ip[4],
                        u16 *src_port);

void net_dump_device(net_device_t *dev);
