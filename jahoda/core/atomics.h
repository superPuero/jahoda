#ifndef jahoda_atomics
#define jahoda_atomics

#define atomic_inc(ptr)       __atomic_add_fetch((ptr), 1, __ATOMIC_SEQ_CST)
#define atomic_dec(ptr)       __atomic_sub_fetch((ptr), 1, __ATOMIC_SEQ_CST)
#define atomic_add(ptr, val)  __atomic_add_fetch((ptr), (val), __ATOMIC_SEQ_CST)
#define atomic_sub(ptr, val)  __atomic_sub_fetch((ptr), (val), __ATOMIC_SEQ_CST)

#define atomic_load(ptr)      __atomic_load_n((ptr), __ATOMIC_SEQ_CST)
#define atomic_store(ptr, v)  __atomic_store_n((ptr), (v), __ATOMIC_SEQ_CST)

#define atomic_cas(ptr, exp_ptr, des) \
    __atomic_compare_exchange_n((ptr), (exp_ptr), (des), false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

#if defined(__x86_64__) || defined(__i386__)

    #define cpu_relax() __builtin_ia32_pause()

#elif defined(__aarch64__) || defined(__arm__)
    #define cpu_relax() __asm__ volatile("yield" ::: "memory")
#else
    #define cpu_relax() 
#endif

#endif