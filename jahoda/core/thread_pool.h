#ifndef jahoda_thread_pool
#define jahoda_thread_pool

#include "types.h"
#include "da.h"
#include "str.h"
#include <pthread.h>

#define thread_pool_max_tasks 1024

da_declare(pthread_t, pthread_da)

typedef struct
{
    void *(*func)(void *);
    void *arg;
}thread_pool_task;

typedef struct
{
    str name;

    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_da threads;

    thread_pool_task task_queue[thread_pool_max_tasks];

    u32 task_count;
    u32 head; 
    u32 tail; 
    
    u32 stop;
} thread_pool;

thread_pool *thread_pool_make(arena *mem, strv name, u32 thread_count);
void *thread_pool_spinup_worker(void *);
void thread_pool_schedule(thread_pool *pool, thread_pool_task task);
void thread_pool_release(thread_pool *pool);

#endif