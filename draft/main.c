#include <jahoda/core/core.h>
#include <jahoda/core/thread_pool.h>
#include <jahoda/core/net.h>

void *task(void*)
{
	char name[128];
	size_t len = 128;
	pthread_getname_np(pthread_self(), (char*)&name, len);
	info("%.*s", len, name);
	return NULL;
}

int main()
{		
	arena *mem = arena_make(.capacity = Gb(1));
	arena *prf_mem = arena_make(.capacity = Mb(1));

	thread_pool *pool = thread_pool_make(
		.mem = mem, 
		.name = strv_from_cstr("http_server_pool"),
		.size = 32
	);
	
	net_context net = net_context_make();

	socket_udp sock = socket_udp_make(&net);
	
	socket_udp_v4_bind(sock, endpoint_v4_make(.address = address_v4_any, .port = 7777));
	
	socket_udp_set_blocking(sock, false);

	while(true)
	{
		scratch packet_scratch = scratch_begin(mem);

		udp_packet_v4_da packets = socket_udp_v4_recv(mem, sock, 2048, 128);

		for da_each(&packets, it)
		{
			if(it->payload.len == 4)
			{
				info("%u", *memv_as(&it->payload, u32));
			}
			else
			{
				info("invalid packet, len: %u", it->payload.len);
			}

			struct { u32 x; u32 y; } val = { 42, 42 };

			socket_udp_v4_send(sock, memv_from(&val), it->endp);
		}		

		scratch_end(packet_scratch);
	}

	net_context_release(&net);
}
