#include "thread_pool.h"
#include <pthread.h>

thread_pool *thread_pool_make(arena *mem, strv name, u32 thread_count)
{
    thread_pool *out = arena_ppush(mem, thread_pool);

    out->name = str_from_view_nt(mem, name);

    pthread_mutex_init(&out->lock, NULL);
    pthread_cond_init(&out->notify, NULL);

    da_resize(mem, &out->threads, thread_count);

    uz index = 0;

    for da_each(&out->threads, it)
    {        
        pthread_create(it, NULL, &thread_pool_spinup_worker, out);

        scratch name_scratch = scratch_begin(mem);         

        pthread_setname_np(*it, str_from_fmt_nt(mem, "(%.*s), thread: %zu", str_fmt(&out->name), index).data);

        scratch_end(name_scratch);

        index++;
    }    

    infol(thread_alloc, "(%.*s) %u threads", strv_fmt(&name), thread_count);

    return out;
}

void *thread_pool_spinup_worker(void *arg)
{
    thread_pool *pool = (thread_pool*)arg;

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

        (*(task.func))(task.arg);      
    }

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
