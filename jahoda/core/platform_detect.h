#ifndef jahoda_platform_detect
#define jahoda_platform_detect

#ifdef _WIN32
#define jahoda_platform_windows
#define jahoda_platform_string "windows64"
#else
#define jahoda_platform_unix
    #ifdef __APPLE__
    #define jahoda_platform_macos
    #define jahoda_platform_string "macos"
    #endif
    #ifdef __linux__
    #define jahoda_platform_string "linux"
    #endif
#endif



#endif