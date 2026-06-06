#include "net/net.h"
#include "mm/heap.h"
#include "libk/log.h"
#include "libk/mem.h"

static Link g_net_sockets;
static int g_next_sock_fd = 1;

int net_socket_init(void)
{
    ListInit(&g_net_sockets);
    g_next_sock_fd = 1;
    return NET_OK;
}

net_socket_t *net_socket_create(net_socket_type_t type)
{
    if (type != NET_SOCK_DGRAM && type != NET_SOCK_STREAM) return NULL;

    net_socket_t *sock = calloc(1, sizeof(*sock));

    if (!sock) return NULL;

    sock->fd = g_next_sock_fd++;
    sock->type = type;
    sock->port = 0;
    sock->device = NULL;
    ListInit(&sock->link);
    ListInit(&sock->rx_queue);
    ListBefore(&g_net_sockets, &sock->link);

    return sock;
}

int net_socket_bind(net_socket_t *sock, u16 port)
{
    if (!sock || port == 0) return NET_EINVAL;

    sock->port = port;

    return NET_OK;
}

int net_socket_close(net_socket_t *sock)
{
    if (!sock) return NET_EINVAL;
    if (sock->link.next && sock->link.prev){ ListRemove(&sock->link);}

    while (!ListIsEmpty(&sock->rx_queue))
    {
        Link *node = sock->rx_queue.next;
        ListRemove(node);
        net_packet_t *pkt = LinkData(node, net_packet_t, link);
        net_packet_free(pkt);
    }

    free(sock);
    return NET_OK;
}

int net_socket_sendto(net_socket_t *sock,
                      const u8 dst_ip[4],
                      u16 dst_port,
                      const void *data,
                      usize len)
{
    (void)dst_ip;
    (void)dst_port;

    if (!sock || !data || len == 0) return NET_EINVAL;

    net_device_t *dev = sock->device ? sock->device : net_find_device("lo");

    if (!dev) return NET_ENOENT;

    return net_send(dev, data, len);
}

int net_socket_recvfrom(net_socket_t *sock,
                        void *buf,
                        usize len,
                        usize *out,
                        u8 src_ip[4],
                        u16 *src_port)
{
    if (!sock || !buf || len == 0) return NET_EINVAL;
    if (ListIsEmpty(&sock->rx_queue)) return NET_EAGAIN;

    Link *node = sock->rx_queue.next;

    ListRemove(node);

    net_packet_t *pkt = LinkData(node, net_packet_t, link);
    usize copied = pkt->len <= len ? pkt->len : len;

    memcpy(buf, pkt->data, copied);

    if (out){ *out = copied;}
    if (src_ip){memset(src_ip, 0, 4);}
    if (src_port){ *src_port = 0;}

    net_packet_free(pkt);

    return NET_OK;
}
