#ifndef jahoda_platform_detect
#define jahoda_platform_detect

#ifdef _WIN32
#define jahoda_platform_windows
#else
#define jahoda_platform_unix
#endif

#ifdef __APPLE__
#define jahoda_platform_macos
#endif

#endif