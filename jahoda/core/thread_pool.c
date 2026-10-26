#include "thread_pool.h"
#include "platform_detect.h"

thread_pool *thread_pool_make_(thread_pool_config config)
{
    thread_pool *out = arena_ppush(config.mem, thread_pool);

    out->size = config.size;

    pthread_mutex_init(&out->lock, NULL);
    pthread_cond_init(&out->notify, NULL);
    pthread_cond_init(&out->completion, NULL);

    da_resize(&out->threads, config.mem, config.size);

    for da_each(it, &out->threads)
    {        
        pthread_create(it, NULL, &thread_pool_spinup_worker, out);
    }    

    infol(thread_alloc, "(%s) %u threads", config.name.data, config.size);

    return out;
}   

void *thread_pool_spinup_worker(void *arg)
{
    thread_pool *pool = (thread_pool*)arg;

    while (true) 
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
        pool->active_task_count++;

        pthread_mutex_unlock(&pool->lock);

        (*(task.func))(task.arg);

        pthread_mutex_lock(&pool->lock);
        pool->active_task_count--;

        if (pool->task_count == 0 && pool->active_task_count == 0)
        {
            pthread_cond_broadcast(&pool->completion);
        }

        pthread_mutex_unlock(&pool->lock);
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

    for da_each(it, &pool->threads)
    {
        pthread_join(*it, NULL);
    }

    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->notify);
    pthread_cond_destroy(&pool->completion);
}

void thread_pool_wait_all(thread_pool *pool)
{   
    pthread_mutex_lock(&pool->lock);

    while(pool->active_task_count > 0 || pool->task_count > 0) 
    { 
        pthread_cond_wait(&pool->completion, &pool->lock);
    }

    pthread_mutex_unlock(&pool->lock);
}