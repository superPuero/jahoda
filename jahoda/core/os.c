#include "os.h"
#include "platform_detect.h"

#ifdef jahoda_platform_windows
    #include <windows.h>

    void *os_mem_reserve(u64 amount) 
    {
        void *result = VirtualAlloc(NULL, amount, MEM_RESERVE, PAGE_NOACCESS);    
        verifyl(os_mem_reserve, result, "memory reserve faied");
        return result;
    }

    void os_mem_commit(void *start, u64 amount)  
    {
        VirtualAlloc(start, amount, MEM_COMMIT, PAGE_READWRITE);
    }

    void os_mem_decommit(void *mem, u64 amount)
    {
        VirtualFree(mem, amount, MEM_DECOMMIT);
    }

    void os_mem_release(void *mem, u64 amount)
    {
        VirtualFree(mem, 0, MEM_RELEASE);
    }
    
    u64 os_mem_fetch_page_size()
    {
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        return sys_info.dwPageSize;
    }


#elif defined(jahoda_platform_unix)
    #include <sys/mman.h>
    #include <unistd.h>
    
    void *os_mem_reserve(u64 pages) 
    {
        uz total_bytes = pages * os_mem_fetch_page_size();    
        void *result = mmap(NULL, total_bytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);    
        verifyl(os_mem_reserve, result != MAP_FAILED, "memory reserve failed");
        return result;
    }

    void os_mem_commit(void *start, u64 amount)  
    {
        mprotect(start, amount, PROT_READ | PROT_WRITE);
    }

    void os_mem_decommit(void *mem, u64 amount)
    {
        madvise(mem, amount, MADV_DONTNEED);
        
        mprotect(mem, amount, PROT_NONE);
    }

    void os_mem_release(void *mem, u64 amount)
    {
        munmap(mem, amount);
    }
    
    u64 os_mem_fetch_page_size()
    {
        return sysconf(_SC_PAGESIZE);
    }

#endif