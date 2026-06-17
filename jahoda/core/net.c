#include "net.h"

#include <jahoda/core/platform_detect.h>

#ifdef jahoda_platform_windows
#include "winsock2.h"
typedef int socklen_t;
#define net_get_error() WSAGetLastError()
#define finna_block WSAEWOULDBLOCK
#else
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#define net_get_error() errno
#define finna_block EAGAIN
#endif

net_context net_context_make()
{
    net_context out = {0};
    
    #ifdef jahoda_platform_windows
        WSADATA wsaData;
        verify(WSAStartup(MAKEWORD(2, 2), &wsaData) == 0, "failed to initialize winsock");
    #endif

    return out;
}

void net_context_release(net_context *ctx)
{
    #ifdef jahoda_platform_windows
	WSACleanup();
    #endif
}

socket_tcp socket_tcp_make(net_context *net)
{    
    socket_tcp out = {0};
    
    out = socket(AF_INET, SOCK_STREAM, 0);

    return out;
}


socket_udp socket_udp_make(net_context *net)
{    
    socket_udp out = {0};
    
    out = socket(AF_INET, SOCK_DGRAM, 0);

    return out;
}

void socket_tcp_release(socket_tcp sock)
{
    #ifdef jahoda_platform_windows
    closesocket(sock);
    #else
    close(sock);
    #endif
}

void socket_udp_release(socket_udp sock)
{
    #ifdef jahoda_platform_windows
    closesocket(sock);
    #else
    close(sock);
    #endif
}


client_tcp_v4 socket_tcp_v4_accept(socket_tcp sock)
{
    client_tcp_v4 out = {0};

    struct sockaddr_in client_addr = {0};
    socklen_t size = sizeof(struct sockaddr_in);
    out.sock = accept(sock, (struct sockaddr*)&client_addr, &size);
	out.endp.address = *(address_v4*)&client_addr.sin_addr.s_addr;
    out.endp.port = ntohs(client_addr.sin_port);
    
    return out;
}

udp_packet_v4_da socket_udp_v4_recv(arena *mem, socket_udp sock, u32 packet_top_size, u32 max_reads)
{
    udp_packet_v4_da out = {0};

    da_reserve(mem, &out, max_reads);

    u32 reads = 0;

    scratch point;

    while (true) 
    {
        da_append(mem, &out, (udp_packet_v4){0});
        udp_packet_v4 *packet = da_last(&out);
        
        point = scratch_begin(mem);
        packet->payload.data = arena_push(mem, packet_top_size, 1, false); 

        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        i32 bytes_read = recvfrom(sock, packet->payload.data, packet_top_size, 0, (struct sockaddr*)&client_addr, &addr_len);    

        if (bytes_read < 0) 
        {
            out.occupied--;
            scratch_end(point);

            int err = net_get_error(); 
            
            if (err == finna_block) 
            {
                break; 
            } 
            else 
            {
                errl(udp, "err");
                break;
            }
        }
        
        packet->payload.len = bytes_read;
        packet->endp.address.one = client_addr.sin_addr.s_addr;
        packet->endp.port = ntohs(client_addr.sin_port);
        if(++reads == max_reads) { break; }
    }

    return out;
}

void socket_udp_set_blocking(socket_udp sock, bool8 blocking)
{
    #ifdef jahoda_platform_windows
        u32 mode = blocking ? 0 : 1;
        ioctlsocket(sock, FIONBIO, &mode);
    #else
        i32 flags = fcntl(sock, F_GETFL, 0);
        
        if (blocking) 
        {
            fcntl(sock, F_SETFL, flags & ~O_NONBLOCK);
        }
        else
        {
            fcntl(sock, F_SETFL, flags | O_NONBLOCK);
        }
    #endif
}

bool8 socket_tcp_v4_bind(socket_tcp sock, endpoint_v4 endpoint)
{
    int opt = 1;
    #ifdef jahoda_platform_windows
        setsockopt(sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*)&opt, sizeof(opt));
    #else
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    #endif
    
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(endpoint.address.one);
    address.sin_port = htons(endpoint.port);

    return bind(sock, (struct sockaddr*)&address, sizeof(address)) != -1;
}

bool8 socket_udp_v4_bind(socket_udp sock, endpoint_v4 endpoint)
{
    int opt = 1;
    #ifdef jahoda_platform_windows
        setsockopt(sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*)&opt, sizeof(opt));
    #else
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    #endif
    
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(endpoint.address.one);
    address.sin_port = htons(endpoint.port);

    return bind(sock, (struct sockaddr*)&address, sizeof(address)) != -1;
}

bool8 socket_tcp_v4_listen(socket_tcp sock, i32 max_connections)
{
    return listen(sock, max_connections) != -1;
}

recv_result socket_tcp_v4_recv(arena *mem, socket_tcp sock, u32 size)
{
    recv_result out = {
        .content = {
            .len = size,
            .data = arena_push(mem, size, 1, true)
        }
    };

    out.status = recv(sock, out.content.data, size, 0);

    return out;
}

i32 socket_udp_v4_send(socket_udp sock, memv data, endpoint_v4 endp)
{
    struct sockaddr_in dest_addr = {0};
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = endp.address.one; 
    dest_addr.sin_port = htons(endp.port);    

    i32 bytes_sent = sendto(sock, data.data, data.len, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    
    if (bytes_sent < 0) 
    {
        int err = net_get_error();

        if (err == finna_block) 
        {
            return 0; 
        } 
        else 
        {
            errl(udp, "failed to send packet");
            return -1;
        }
    }
    
    return bytes_sent;
}

i32 socket_tcp_send(socket_tcp sock, memv data)
{
    return send(sock, data.data, data.len, 0);
}