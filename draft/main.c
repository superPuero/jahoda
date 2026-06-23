#include <jahoda/core/core.h>
#include <jahoda/core/thread_pool.h>
#include <jahoda/core/net.h>
#include <jahoda/core/sht.h>
#include "cmd_line_args.h"

typedef struct server_context server_context;

typedef struct
{
	tcp_client_v4 client;
	server_context *context;
} serve_task;
			
sht_declare(serve_task, 32, 12, serve_task_table);

typedef struct server_context
{
	serve_task_table task_table;
	pthread_mutex_t mutex;
};

void client_serve(arena *mem, serve_task *task)
{
	info("connected %u.%u.%u.%u:%u", endpoint_v4_fmt(&task->client.endp));
	
	while(true)
	{
		scratch temp = scratch_begin(mem);			

		tcp_packet packet = tcp_socket_recv(mem, task->client.sock, 1024);

		if(packet.status == socket_status_success)
		{			
			log_sync_lock();
			printf("message from %u.%u.%u.%u:%u: %.*s", endpoint_v4_fmt(&task->client.endp), strv_fmt(&packet.payload));
			log_sync_unlock();
		}
		else
		{
			break;
		}		 
		
		scratch_end(temp);
	}

	str key = str_from_fmt(mem, "%u.%u.%u.%u:%u", endpoint_v4_fmt(&task->client.endp));
	strv keyv = strv_from_str(&key);

	pthread_mutex_lock(&task->context->mutex);

	sht_remove(&task->context->task_table, keyv);

	pthread_mutex_unlock(&task->context->mutex);

	info("disconnected %u.%u.%u.%u:%u", endpoint_v4_fmt(&task->client.endp));
}

void *client_serve_task(thread_context *tctx, void *arg)
{
	client_serve(tctx->mem, (serve_task*)(arg));
	return NULL;
}

int main(int argc, char **argv)
{      
	cmd_line_args cmd_args = cmd_line_args_parse(argc, argv); 

	if(cmd_args.h)
	{
		cmd_line_args_display();
		return 0;
	}

	arena *thread_pool_mem = arena_make(
		.name = cstrv("thread_pool_mem"),
		.capacity = Gb(1)
	);
	
	arena *temp_mem = arena_make(
		.name = cstrv("client_mem"),
		.capacity = Gb(1)
	);
	
	net_context net = net_context_make();

	tcp_socket server = tcp_socket_make(&net);

	thread_pool *server_thread_pool = thread_pool_make(
		.mem = thread_pool_mem, 
		.name = strv_from_cstr("server_thread_pool"), 
		.size = 128,
		.per_thread_mem = Gb(1)
	);

	endpoint_v4 *endp = endpoint_v4_from_strv(temp_mem, cstrv(cmd_args.endp));

	if(!endp)
	{
		err("%s is not a valid endpooint", cmd_args.endp);
	}
	
	tcp_socket_v4_bind(server, *endp);
	tcp_socket_listen(server, 1024);
	
	server_context server_ctx = {
		.mutex = PTHREAD_MUTEX_INITIALIZER,		
	};	

	while(1)
	{
		scratch s = scratch_begin(temp_mem);

		tcp_client_v4 client = tcp_socket_v4_accept(server);
		str key = str_from_fmt(temp_mem, "%u.%u.%u.%u:%u", endpoint_v4_fmt(&client.endp));
		strv keyv = strv_from_str(&key);

		pthread_mutex_lock(&server_ctx.mutex);

		sht_insert(&server_ctx.task_table, keyv, (serve_task){.client = client, .context = &server_ctx});

		pthread_mutex_unlock(&server_ctx.mutex);

		serve_task *serve_task_ptr;
		sht_get(&server_ctx.task_table, keyv, serve_task_ptr);
		
		thread_pool_schedule(
			server_thread_pool, 
			(thread_pool_task){
				.arg = serve_task_ptr,
				.func = &client_serve_task,
			}
		);

		scratch_end(s);
	}
}
