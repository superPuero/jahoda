#include "thread.h"
#include "pthread.h"

// void threads_launch(threadv group ref, void* (*fn)(void *data))
// {
    // for each_index(i, group.count)
    // {
        // pthread_create(&group.threads[i].handle, NULL, NULL, fn);
    // }
// }
// void threads_join(threadv group);