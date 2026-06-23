#include "net.h"

#include <jahoda/core/platform_detect.h>

#ifdef jahoda_platform_windows
    #include "winsock2.h"
    typedef int socklen_t;
    #define net_get_error() WSAGetLastError()
    #define conn_whould_block WSAEWOULDBLOCK
    #define conn_timeout WSAETIMEDOUT
    #define conn_reset WSAECONNRESET
    #define conn_no_connection WSAENOTCONN
#else
    #include <fcntl.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <errno.h>
    #define net_get_error() errno
    #define conn_whould_block EAGAIN
    #define conn_timeout ETIMEDOUT
    #define conn_reset ECONNRESET
    #define conn_no_connection ENOTCONN
#endif

address_v4 *address_v4_from_strv(arena *mem, strv view)
{
    scratch s = scratch_begin(mem);

    address_v4 *out = arena_ppush(mem, address_v4);

    u16 octets[4] = {0};
    u8 current = 0;
    u8 done = 0;
    u8 octet = 0;

    while(1)
    {        
        if(isdigit(view.data[current]))
        {
            octets[done] *= 10;
            octets[done] += view.data[current] - '0'; 

            if(octets[done] > UINT8_MAX) { break; }   

            octet = done;   
        }
        else if(view.data[current] == '.')
        {
            done++;
            if(done == 4) { break; }
        }

        if(view.len == current) { break; }

        current++;
    }

    if(done == 3 && octet == 3 && (current == view.len))
    {
        out->four[0] = octets[0];
        out->four[1] = octets[1];
        out->four[2] = octets[2];
        out->four[3] = octets[3];

        return out;
    }

    scratch_end(s);
    return NULL;
}

endpoint_v4 *endpoint_v4_from_strv(arena *mem, strv view)
{
    scratch s = scratch_begin(mem);

    endpoint_v4 *out = arena_fpush(mem, endpoint_v4);

    u8 current = 0;
    u32 port = 0;
    u8 done = 0;
    bool8 port_found = 0;

    while(1) 
    {

        if(view.len == current){ done++; break; }

        if(done == 1)
        {
            port *= 10;
            port += view.data[current] - '0'; 
            if(port > UINT16_MAX)
            {
                break;
            }
            port_found = true;
        }

        if(view.data[current] == ':')
        {
            scratch addr_scratch = scratch_begin(mem);

            address_v4 *addr = address_v4_from_strv(mem, strv_make(view.data, current));

            if(!addr)
            {
                break;
            }
            else
            {
                out->address = *addr;
                done++;
            }

            scratch_end(addr_scratch);
        }

         current++; 
    }

    if(done == 2 && port_found)
    {
        out->port = port;
        return out;
    }

    scratch_end(s);

    return NULL;

}

str endpoint_v4_to_str(arena *mem, endpoint_v4 *endp)
{
    return str_from_fmt(mem, "%u.%u.%u.%u:%u", endpoint_v4_fmt(endp));
}

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

socket_status socket_status_check(i32 status)
{
    if(status > 0)
    {
        return socket_status_success;
    }
    else if(status == 0)
    {
        return socket_status_disconnect;
    }
    else if(status < 0)
    {
        i32 err = net_get_error(); 
            
        if (err == conn_whould_block) 
        {
            return socket_status_would_block; 
        }
    }
}

tcp_socket tcp_socket_make(net_context *net)
{    
    tcp_socket out = {0};
    
    out = socket(AF_INET, SOCK_STREAM, 0);

    return out;
}


udp_socket udp_socket_make(net_context *net)
{    
    udp_socket out = {0};
    
    out = socket(AF_INET, SOCK_DGRAM, 0);

    return out;
}

void tcp_socket_release(tcp_socket sock)
{
    #ifdef jahoda_platform_windows
    closesocket(sock);
    #else
    close(sock);
    #endif
}

void udp_socket_release(udp_socket sock)
{
    #ifdef jahoda_platform_windows
    closesocket(sock);
    #else
    close(sock);
    #endif
}


tcp_client_v4 tcp_socket_v4_accept(tcp_socket sock)
{
    tcp_client_v4 out = {0};

    struct sockaddr_in client_addr = {0};
    socklen_t size = sizeof(struct sockaddr_in);
    out.sock = accept(sock, (struct sockaddr*)&client_addr, &size);
	out.endp.address = *(address_v4*)&client_addr.sin_addr.s_addr;
    out.endp.port = ntohs(client_addr.sin_port);
    
    return out;
}

udp_packet_v4_bundle udp_socket_v4_recv_nonblocking(arena *mem, udp_socket sock, u32 packet_top_size, u32 max_reads)
{
    udp_packet_v4_bundle out = {0};
    da_reserve(mem, &out.entries, max_reads);

    u32 reads = 0;

    scratch point;

    while (true) 
    {
        da_append(mem, &out.entries, (udp_packet_v4){0});
        udp_packet_v4 *packet = da_last(&out.entries);
        
        point = scratch_begin(mem);
        packet->payload.data = arena_push(mem, packet_top_size, 1, false); 

        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        i32 res = recvfrom(sock, packet->payload.data, packet_top_size, 0, (struct sockaddr*)&client_addr, &addr_len);    
        
        if (res < 0) 
        {
            out.entries.occupied--;
            out.status = socket_status_check(res);
            scratch_end(point);
            break;
        }
        
        packet->payload.len = res;
        arena_pop(mem, packet_top_size - res);

        packet->endp.address.one = ntohl(client_addr.sin_addr.s_addr);
        packet->endp.port = ntohs(client_addr.sin_port);

        if(++reads == max_reads) { break; }
    }

    return out;
}

void tcp_socket_set_blocking(tcp_socket sock, bool8 blocking)
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


void udp_socket_set_blocking(udp_socket sock, bool8 blocking)
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

bool8 tcp_socket_v4_bind(tcp_socket sock, endpoint_v4 endpoint)
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

bool8 udp_socket_v4_bind(udp_socket sock, endpoint_v4 endpoint)
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

bool8 tcp_socket_listen(tcp_socket sock, i32 max_connections)
{
    return listen(sock, max_connections) != -1;
}

tcp_packet tcp_socket_recv(arena *mem, tcp_socket sock, u32 max_packet_size)
{
    tcp_packet out = {
        .payload = {
            .len = max_packet_size,
            .data = arena_push(mem, max_packet_size, 1, true)
        }
    };

    i32 res = recv(sock, out.payload.data, max_packet_size, 0);

    if (res > 0) 
    {
        out.payload.len = res;
        out.status = socket_status_check(res);
        arena_pop(mem, max_packet_size - res);
    } 
    else 
    {
        out.payload.len = 0;
        out.status = socket_status_check(res);
        arena_pop(mem, max_packet_size);
        out.payload.data = NULL;
    }

    return out;
}

i32 udp_socket_v4_send(udp_socket sock, memv data, endpoint_v4 endp)
{
    struct sockaddr_in dest_addr = {0};
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = endp.address.one; 
    dest_addr.sin_port = htons(endp.port);    

    i32 bytes_sent = sendto(sock, data.data, data.len, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    
    if (bytes_sent < 0) 
    {
        int err = net_get_error();

        if (err == conn_whould_block) 
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

i32 tcp_socket_send(tcp_socket sock, memv data)
{
    return send(sock, data.data, data.len, 0);
}