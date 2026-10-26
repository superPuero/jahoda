#ifndef jahoda_thread_pool
#define jahoda_thread_pool

#include "types.h"
#include "da.h"
#include <pthread.h>

#define thread_pool_max_tasks 128

da_declare(pthread_t, pthread_da)

typedef struct
{
    void *(*func)(void *);
    void *arg;
} thread_pool_task;

#define thread_pool_task_declare(name, arg_, ...)\
typedef struct arg_ name##_args;\
void *name(void *in)\
{\
    name##_args *args = (name##_args*)in;\
    __VA_ARGS__;\
}\
thread_pool_task name##_task_make(name##_args *args)\
{\
    return (thread_pool_task) {.func = name, .arg = args};\
}

typedef struct
{
    u32 size;

    pthread_mutex_t lock;

    pthread_cond_t notify;
    pthread_cond_t completion;

    pthread_da threads;

    thread_pool_task task_queue[thread_pool_max_tasks];

    u32 task_count;
    u32 active_task_count;

    u32 head; 
    u32 tail; 
    
    u32 stop;
} thread_pool;

typedef struct
{
    arena *mem;
    text64 name; 
    u32 size;    
} thread_pool_config;

thread_pool *thread_pool_make_(thread_pool_config config);
#define thread_pool_make(...) thread_pool_make_((thread_pool_config){__VA_ARGS__})
void *thread_pool_spinup_worker(void *);
void thread_pool_schedule(thread_pool *pool, thread_pool_task task);
void thread_pool_release(thread_pool *pool);
void thread_pool_wait_all(thread_pool *pool);


#endif