#include "os.h"
#include "platform_detect.h"

#ifdef jahoda_platform_windows
    #include <windows.h>

    void *os_mem_reserve_pages(u64 pages) 
    {
        uz total_bytes = pages * os_page_size;    
        void *result = VirtualAlloc(NULL, total_bytes, MEM_RESERVE, PAGE_NOACCESS);    
        verifyl(os_mem_reserve_pages, result, "memory reserve faied");
        return result;
    }

    void os_mem_commit_pages(void *start, u64 pages)  
    {
        VirtualAlloc(start, pages * os_page_size, MEM_COMMIT, PAGE_READWRITE);
    }

    void os_mem_decommit_pages(void *mem, u64 pages)
    {
        VirtualFree(mem, pages * os_page_size, MEM_DECOMMIT);
    }

    void os_mem_release_pages(void *mem, u64 pages)
    {
        VirtualFree(mem, 0, MEM_RELEASE);
    }
    
#elif defined(jahoda_platform_unix)
    #include <sys/mman.h>
    
    void *os_mem_reserve_pages(u64 pages) 
    {
        uz total_bytes = pages * os_page_size;    
        void *result = mmap(NULL, total_bytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);    
        verify(result != MAP_FAILED, "memory reserve failed");
        return result;
    }

    void os_mem_commit_pages(void *start, u64 pages)  
    {
        mprotect(start, pages * os_page_size, PROT_READ | PROT_WRITE);
    }

    void os_mem_decommit_pages(void *mem, u64 pages)
    {
        uz total_bytes = pages * os_page_size;
        
        madvise(mem, total_bytes, MADV_DONTNEED);
        
        mprotect(mem, total_bytes, PROT_NONE);
    }

    void os_mem_release(void *mem, u64 pages)
    {
        munmap(mem, pages * os_page_size);
    }

#endif