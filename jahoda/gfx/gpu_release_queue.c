#include "gpu_release_queue.h"

void gpu_release_queue_append(gpu_release_queue *queue, gpu_release_queue_task task)
{
    verify(queue->current < gpu_release_queue_capacity);
    queue->tasks[queue->current++] = task;
}

void gpu_release_queue_exec(gpu_release_queue *queue, gpu_device *device)
{
    for each_index(i, queue->current)
    {
        queue->tasks[i].func(device->handle, queue->tasks[i].handle, NULL);
    }

    queue->current = 0;
}