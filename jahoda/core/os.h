#ifndef jahoda_os
#define jahoda_os


#include "utils.h"
#include "types.h"

void *os_mem_reserve(u64 amount);
void os_mem_commit(void *start, u64 amount);
void os_mem_decommit(void *start, u64 amount);
void os_mem_release(void *mem, u64 amount);
u64 os_mem_fetch_page_size();

#endif