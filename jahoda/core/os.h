#ifndef jahoda_os
#define jahoda_os

#include "utils.h"
#include "types.h"

typedef u64 os_mem_flags;

typedef enum
{
    os_mem_flag_noaccess = 1 << 0,
    os_mem_flag_read = 2 << 0,
    os_mem_flag_read_write = 3 << 0,
    os_mem_flag_execute = 4 << 0,
    os_mem_flag_execute_read = os_mem_flag_execute | os_mem_flag_read,
    os_mem_flag_execute_read_write = os_mem_flag_execute | os_mem_flag_read_write
} os_mem_flag_;

void *os_mem_reserve(u64 amount);
void os_mem_commit(void *start, u64 amount, os_mem_flags flags);
void os_mem_decommit(void *start, u64 amount);
void os_mem_release(void *mem, u64 amount);
u64 os_mem_fetch_page_size();
u64 os_mem_fetch_allocation_granularity();

#endif