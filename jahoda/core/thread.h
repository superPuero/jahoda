#ifndef jahoda_thread
#define jahoda_thread

#include <pthread.h>
#include "types.h"
#include "arena.h"

typedef struct {    
    pthread_t handle;
    uz data;    
} thread;

typedef struct{
    thread *threads;
    uz count;
} threadv;

void threads_launch_group(threadv group, void* (*fn)(void *data), void *data);
void threads_join_group(threadv group);

#endif