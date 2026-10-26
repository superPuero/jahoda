#include "os.h"
#include "platform_detect.h"

#ifdef jahoda_platform_windows
        #undef _WIN32_WINNT
        #define _WIN32_WINNT 0x0A00

        #include <windows.h>
        #include <memoryapi.h>

        internal u64 _os_mem_flags_to_native(os_mem_flags flags)
        {  
            if((flags & ~os_mem_flag_noaccess) == 0)
            {
                return PAGE_NOACCESS;
            }        
            else if((flags & ~(os_mem_flag_read)) == 0)
            {
                return PAGE_READONLY;
            }
            else if((flags & ~os_mem_flag_read_write) == 0)
            {
                return PAGE_READWRITE;
            }
            else if((flags & ~os_mem_flag_execute) == 0)
            {
                return PAGE_EXECUTE;
            }            
            else if((flags & ~os_mem_flag_execute_read) == 0)
            {
                return PAGE_EXECUTE_READ;
            }            
            else if((flags & ~os_mem_flag_execute_read_write) == 0)
            {
                return PAGE_EXECUTE_READWRITE;
            }            

            dbg_verifyl(_os_mem_flags_to_native, false, "invalid mem flag combination %u", (u32)flags);
        }

        void *os_mem_reserve(u64 amount) 
        {    
            void *result = VirtualAlloc(NULL, amount, MEM_RESERVE, PAGE_NOACCESS);    
            verifyl(os_mem_reserve, result, "memory reserve faied");
            return result;
        }

        void os_mem_commit(void *start, u64 amount, os_mem_flags flags)  
        {
            VirtualAlloc(start, amount, MEM_COMMIT, _os_mem_flags_to_native(flags));
        }

        void os_mem_decommit(void *mem, u64 amount)
        {
            VirtualFree(mem, amount, MEM_DECOMMIT);
        }

        void os_mem_release(void *mem, u64 amount)
        {
            VirtualFree(mem, 0, MEM_RELEASE);
        }
        
        os_hardware_info os_fetch_hardware_info()
        {            
            os_hardware_info info = {0};
            SYSTEM_INFO sys_info;
            GetSystemInfo(&sys_info);
            info.logical_processors = sys_info.dwNumberOfProcessors;
            info.page_size          = sys_info.dwPageSize;            

            MEMORYSTATUSEX mem_info;
            mem_info.dwLength = sizeof(MEMORYSTATUSEX);
            verify(GlobalMemoryStatusEx(&mem_info));            
            info.total_physical_memory = mem_info.ullTotalPhys;

            return info;
        }

#elif defined(jahoda_platform_unix)
    #include <sys/mman.h>
    #include <unistd.h>

    static u64 *_os_mem_flags_to_native(os_mem_flags flags)
    {  
        if((flags & ~os_mem_flag_noaccess) == 0)
        {
            return PROT_NONE;
        }        
        else if((flags & ~(os_mem_flag_read)) == 0)
        {
            return PROT_READ;
        }
        else if((flags & ~os_mem_flag_read_write) == 0)
        {
            return PROT_READ | PROT_WRITE;
        }
        else if((flags & ~os_mem_flag_execute) == 0)
        {
            return PROT_EXEC;
        }            
        else if((flags & ~os_mem_flag_execute_read) == 0)
        {
            return PROT_READ | PROT_EXEC;
        }            
        else if((flags & ~os_mem_flag_execute_read_write) == 0)
        {
            return PROT_READ | PROT_WRITE | PROT_EXEC;
        }            

        dbg_verifyl(_os_mem_flags_to_native, false, "invalid mem flag combination %u", (u32)flags);
    }

    
    void *os_mem_reserve(u64 pages) 
    {
        uz total_bytes = pages * os_fetch_hardware_info().page_size;    
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
    
    os_hardware_info os_fetch_hardware_info()
    {        
        os_hardware_info info = {0};

        info.logical_processors = sysconf(_SC_NPROCESSORS_ONLN);
        dbg_verify(info.logical_processors);
        
        info.page_size = sysconf(_SC_PAGESIZE);
        dbg_verify(info.page_size);
        
        u64 phys_pages = sysconf(_SC_PHYS_PAGES);
        dbg_verify(phys_pages);
        info.total_physical_memory = phys_pages * info.page_size;            

        info.cache_line_size = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
        dbg_verify(info.cache_line_size);

        return info;            
    }   
#endif