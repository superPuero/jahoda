#ifndef jahoda_net
#define jahoda_net

#include "types.h"
#include "utils.h"
#include "da.h"
#include "str.h"


#define socket_max_connections 4096

#define socket_invalid (socket_fd)(~0)

typedef struct {} net_context;

net_context net_context_make();
void net_context_release(net_context *ctx);

typedef u64 tcp_socket;
typedef u64 udp_socket;

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

address_v4 *address_v4_from_strv(arena *mem, strv view);
endpoint_v4 *endpoint_v4_from_strv(arena *mem, strv view);
str endpoint_v4_to_str(arena *mem, endpoint_v4 *endp);

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
    tcp_socket sock;
} tcp_client_v4;

typedef struct
{
    endpoint_v4 endp;
    memv payload;
} udp_packet_v4;

typedef enum
{
    socket_status_success = 0,
    socket_status_disconnect = 1,
    socket_status_timeout = 2,
    socket_status_retry = 3,
    socket_status_reset = 4,
    socket_status_would_block = 5,
    socket_status_no_connection = 6,
    socekt_status_there_is_more = 7
} socket_status;

typedef struct
{
    memv payload;
    i32 status;
} tcp_packet;

da_declare(udp_packet_v4, udp_packet_v4_da);

typedef struct
{
    udp_packet_v4_da entries;
    socket_status status;
} udp_packet_v4_bundle;

socket_status socket_status_check(i32 status);

tcp_socket tcp_socket_make(net_context *net);
udp_socket udp_socket_make(net_context *net);

void tcp_socket_release(tcp_socket sock);
void udp_socket_release(udp_socket sock);

tcp_client_v4 tcp_socket_v4_accept(tcp_socket sock);
udp_packet_v4_bundle udp_socket_v4_recv_nonblocking(arena *mem, udp_socket sock, u32 packet_top_size, u32 max_reads);

void tcp_socket_set_blocking(tcp_socket sock, bool8 blocking);
void udp_socket_set_blocking(udp_socket sock, bool8 blocking);

bool8 tcp_socket_v4_bind(tcp_socket sock, endpoint_v4 endpoint);
bool8 udp_socket_v4_bind(udp_socket sock, endpoint_v4 endpoint);

bool8 tcp_socket_listen(tcp_socket sock, i32 max_connections);

tcp_packet tcp_socket_recv(arena *mem, tcp_socket sock, u32 packet_top_size);
i32 udp_socket_v4_send(udp_socket sock, memv data, endpoint_v4 endp);

i32 tcp_socket_send(tcp_socket sock, memv data);

#endif