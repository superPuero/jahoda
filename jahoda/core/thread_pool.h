#ifndef jahoda_thread_pool
#define jahoda_thread_pool

#include "types.h"
#include "da.h"
#include "str.h"
#include <pthread.h>

#define thread_pool_max_tasks 65565

da_declare(pthread_t, pthread_da)

typedef struct
{
    arena *mem;
} thread_context;

typedef struct
{
    void *(*func)(thread_context *, void *);
    void *arg;
} thread_pool_task;

typedef struct
{
    arena *mem;
    str name;

    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_da threads;

    thread_pool_task task_queue[thread_pool_max_tasks];

    u32 task_count;
    u32 head; 
    u32 tail; 
    u32 per_thread_mem;
    
    u32 stop;
} thread_pool;

typedef struct
{
    arena *mem;
    strv name; 
    u32 size;    
    u32 per_thread_mem;
} thread_pool_config;

thread_pool *thread_pool_make_(thread_pool_config config);
#define thread_pool_make(...) thread_pool_make_((thread_pool_config){__VA_ARGS__})
void *thread_pool_spinup_worker(void *);
void thread_pool_schedule(thread_pool *pool, thread_pool_task task);
void thread_pool_release(thread_pool *pool);

#endif