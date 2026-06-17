#ifndef jahoda_os
#define jahoda_os


#include "utils.h"
#include "types.h"

// @todo: this is big assumption, may be passing page size or host_info
#define os_page_size Kb(4)

void *os_mem_reserve_pages(u64 pages);
void os_mem_commit_pages(void *start, u64 ages);
void os_mem_decommit_pages(void *start, u64 ages);
void os_mem_release_pages(void *mem, u64 pages);

#endif