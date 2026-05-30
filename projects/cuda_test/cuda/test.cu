#include "test_bridge.h"
#include <cuda_runtime.h>
#include <stdio.h>

__global__ void add_one_kernel(float* data, int size) 
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) 
    {
        data[idx] += 1.0f;
    }
}

extern "C" int init_cuda(void) 
{
    int device_count = 0;
    cudaError_t err = cudaGetDeviceCount(&device_count);
    
    if (err != cudaSuccess || device_count == 0) {
        printf("CUDA Error: No CUDA-capable devices found.\n");
        return 0;
    }
    
    cudaSetDevice(0);
    printf("CUDA Initialized: Found %d GPU(s). Using Device 0.\n", device_count);
    return 1;
}

extern "C" void test(float* host_array, int size) 
{
    float* d_data = NULL;
    size_t bytes = size * sizeof(float);

    cudaMalloc((void**)&d_data, bytes);
    
    cudaMemcpy(d_data, host_array, bytes, cudaMemcpyHostToDevice);

    int threads_per_block = 256;
    int blocks_per_grid = (size + threads_per_block - 1) / threads_per_block;
    
    add_one_kernel<<<blocks_per_grid, threads_per_block>>>(d_data, size);

    cudaMemcpy(host_array, d_data, bytes, cudaMemcpyDeviceToHost);

    cudaFree(d_data);
}