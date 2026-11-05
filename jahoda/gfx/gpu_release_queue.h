#ifndef jahoda_gpu_release_queue
#define jahoda_gpu_release_queue


#include <jahoda/gfx/gpu_glfw.h>
#include <jahoda/gfx/gpu_device.h>
#include <jahoda/core/core.h>

#define gpu_release_queue_capacity 64
#define gpu_release_defer(queue, handle_, func_) gpu_release_queue_append(queue, (gpu_release_queue_task){ .func = (gpu_release_resource_func*)(void*)&func_, .handle = (u64)handle_} ); 

typedef void (VKAPI_PTR gpu_release_resource_func)(VkDevice, uint64_t, const VkAllocationCallbacks*);

typedef struct
{
    gpu_release_resource_func *func;
    u64 handle;
} gpu_release_queue_task;

typedef struct
{
    gpu_release_queue_task tasks[gpu_release_queue_capacity];
    uz current;
} gpu_release_queue;

void gpu_release_queue_append(gpu_release_queue *queue, gpu_release_queue_task task);
void gpu_release_queue_exec(gpu_release_queue *queue, gpu_device *device);

#endif
