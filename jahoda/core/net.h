#ifndef jahoda_net
#define jahoda_net

#include "types.h"
#include "utils.h"
#include "da.h"

#define socket_max_connections 4096

#define socket_invalid (socket_fd)(~0)

typedef struct {} net_context;

net_context net_context_make();
void net_context_release(net_context *ctx);

typedef u64 socket_tcp;
typedef u64 socket_udp;

typedef union
{
    u8 four[4];
    u32 one;
} address_v4;

#define address_v4_fmt(addr) (addr)->four[0], (addr)->four[1], (addr)->four[2], (addr)->four[3]
#define address_v4_any (address_v4){0}

typedef union
{
    u8 sixteen[16];
    u16 eight[8];
    u32 four[4];
    u64 two[2];
} address_v6;

typedef struct
{
    address_v4 address;
    u16 port;
} endpoint_v4;

#define endpoint_v4_fmt(endp) (endp)->address.four[0], (endp)->address.four[1], (endp)->address.four[2], (endp)->address.four[3], (endp)->port
#define endpoint_v4_make(...) (endpoint_v4){__VA_ARGS__}

typedef struct
{
    address_v6 address;
    u16 port;
} endpoint_v6;

typedef struct
{
    i32 status;
    memv content;
} recv_result;

typedef struct
{
    endpoint_v4 endp;    
    socket_tcp sock;
} client_tcp_v4;

typedef struct
{
    endpoint_v4 endp;
    memv payload;
} udp_packet_v4;

da_declare(udp_packet_v4, udp_packet_v4_da);

socket_tcp socket_tcp_make(net_context *net);
socket_udp socket_udp_make(net_context *net);

void socket_tcp_release(socket_tcp sock);
void socket_udp_release(socket_udp sock);

client_tcp_v4 socket_tcp_v4_accept(socket_tcp sock);
udp_packet_v4_da socket_udp_v4_recv(arena *mem, socket_udp sock, u32 packet_top_size, u32 max_reads);

void socket_udp_set_blocking(socket_udp sock, bool8 blocking);

bool8 socket_tcp_v4_bind(socket_tcp sock, endpoint_v4 endpoint);
bool8 socket_udp_v4_bind(socket_udp sock, endpoint_v4 endpoint);

bool8 socket_tcp_listen(socket_tcp sock, i32 max_connections);

recv_result socket_tcp_recv(arena *mem, socket_tcp sock, u32 size);
i32 socket_udp_v4_send(socket_udp sock, memv data, endpoint_v4 endp);

i32 socket_tcp_send(socket_tcp sock, memv data);

#endif