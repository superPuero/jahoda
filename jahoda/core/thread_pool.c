#include "thread_pool.h"
#include "platform_detect.h"

#ifdef jahoda_platform_apple
    #define pthread_setname_func(thread_id, ...) pthread_setname_np(__VA_ARGS__)
#else
    #define pthread_setname_func(thread_id, ...) pthread_setname_np(thread_id, __VA_ARGS__)
#endif

thread_pool *thread_pool_make_(thread_pool_config config)
{
    thread_pool *out = arena_ppush(config.mem, thread_pool);

    out->name = str_from_view_nt(config.mem, config.name);
    out->per_thread_mem = config.per_thread_mem;
    pthread_mutex_init(&out->lock, NULL);
    pthread_cond_init(&out->notify, NULL);

    da_resize(config.mem, &out->threads, config.size);

    uz index = 0;

    for da_each(&out->threads, it)
    {        
        pthread_create(it, NULL, &thread_pool_spinup_worker, out);

        index++;
    }    

    infol(thread_alloc, "(%.*s) %u threads", strv_fmt(&config.name), config.size);

    return out;
}

void *thread_pool_spinup_worker(void *arg)
{
    thread_pool *pool = (thread_pool*)arg;

    // @todo:     
    u8 buffer[64];   
    snprintf(&buffer, 63, "(%.*s), thread: %u", str_fmt(&pool->name), pthread_self());

    pthread_setname_func(pthread_self(), buffer);

    thread_context context = {
        .mem = arena_make(.name = strv_from_cstr(buffer), .capacity = pool->per_thread_mem)
    };
    
    while (1) 
    {
        pthread_mutex_lock(&pool->lock);

        while (pool->task_count == 0 && !pool->stop)
        {
            pthread_cond_wait(&pool->notify, &pool->lock);
        }

        if (pool->stop && pool->task_count == 0)
        {
            pthread_mutex_unlock(&pool->lock);
            break; 
        }

        thread_pool_task task = pool->task_queue[pool->head];
        pool->head = (pool->head + 1) % thread_pool_max_tasks; 
        pool->task_count--;

        pthread_mutex_unlock(&pool->lock);

        (*(task.func))(&context, task.arg);   
        
        arena_reset(context.mem);
    }

    arena_release(context.mem);

    return NULL;
}

void thread_pool_schedule(thread_pool *pool, thread_pool_task task)
{
    pthread_mutex_lock(&pool->lock);

    verify(pool->task_count < thread_pool_max_tasks);
    
    pool->task_queue[pool->tail] = task;
    pool->tail = (pool->tail + 1) % thread_pool_max_tasks;
    
    pool->task_count++;

    pthread_mutex_unlock(&pool->lock);

    pthread_cond_signal(&pool->notify);
}

void thread_pool_release(thread_pool *pool)
{
    pthread_mutex_lock(&pool->lock);
    pool->stop = 1; 
    pthread_mutex_unlock(&pool->lock);

    pthread_cond_broadcast(&pool->notify);

    for da_each(&pool->threads, it)
    {
        pthread_join(*it, NULL);
    }

    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->notify);
}
