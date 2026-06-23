#ifndef jahoda_utils
#define jahoda_utils

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <time.h>
#include "platform_detect.h"
#include "types.h"

#define global static
#define internal static

void log_sync_enable();
void log_sync_lock();
void log_sync_unlock();
void log_sync_disable();

#define Kb(expr) ((expr) * 1024ULL)
#define Mb(expr) (Kb(expr) * 1024ULL)
#define Gb(expr) (Mb(expr) * 1024ULL)
#define Tb(expr) (Gb(expr) * 1024ULL)

#define as_Kb(expr) ((expr) / 1024.0)
#define as_Mb(expr) (as_Kb(expr) / 1024.0)
#define as_Gb(expr) (as_Mb(expr) / 1024.0)
#define as_Tb(expr) (as_Gb(expr) / 1024.0)

#define bit(x) (1ull << x)

#ifndef NDEBUG
#define jahoda_debug
#endif

#define concat_helper(x, y) x##y
#define concat(x, y) concat_helper(x, y)


#define each_index_range(index_var_name, from, to)\
(uz index_var_name = from; index_var_name < to; index_var_name++)

#define each_index(index_var_name, count) each_index_range(index_var_name, 0, count)

#define unique_symbol(...) __VA_ARGS__##__COUNTER__	

#define static_assert(expr) typedef int static_assert_##__COUNTER__[(expr) ? 1 : -1]

#define arrsize(arr) (sizeof(arr)/sizeof(arr[0]))

#define ansi_esq_seq "\033"

#define ansi_text_color_green "32"
#define ansi_text_color_red "31"
#define ansi_text_color_yellow "33"
#define ansi_text_color_default "39"

#if defined(__GNUC__) || defined(__clang__)
    #define trap() __builtin_trap()
#else
    #error "trap is not supported on this compiler"
#endif

#ifdef jahoda_platform_windows
    #include <io.h>
    #define jahoda_isatty _isatty
    #define jahoda_fileno _fileno
#else
    #include <unistd.h>
    #define jahoda_isatty isatty
    #define jahoda_fileno fileno
#endif

#define print_colored_if_isatty(color, ...)\
do\
{\
	if(jahoda_isatty(jahoda_fileno(stdout)))\
	{\
		fprintf(stdout,\
			ansi_esq_seq "[" color "m" \
			__VA_ARGS__ \
			ansi_esq_seq "[" ansi_text_color_default "m"\
		);\
	}\
	else\
	{\
		fprintf(stdout,\
			__VA_ARGS__\
		);\
	}\
}while(0)

// #define jahoda_disable_info

#ifndef jahoda_disable_info
#define infol(label, ...)\
do\
{\
	log_sync_lock();\
	print_colored_if_isatty(ansi_text_color_green, "info" " (" #label ") " "[" __DATE__ " " __TIME__ "]" ": ");\
	fprintf(stdout, "" __VA_ARGS__);\
	fprintf(stdout, "\n");\
	log_sync_unlock();\
}while(0)
#define info(...) infol(-, "" __VA_ARGS__)
#else
#define infol(label, ...)
#define info(...)
#endif

#define warnl(label, ...)\
do\
{\
	log_sync_lock();\
	print_colored_if_isatty(ansi_text_color_yellow, "warn"  " (" #label ") "  "[" __DATE__ " " __TIME__ "]" ": ");\
	fprintf(stdout, "" __VA_ARGS__);\
	fprintf(stdout, "\n");\
	log_sync_unlock();\
}while(0)

#define warn(...) warnl(-, "" __VA_ARGS__);

#define errl(label, ...)\
do\
{\
	log_sync_lock();\
	print_colored_if_isatty(ansi_text_color_red, "err" " (" #label ") " "[" __DATE__ " " __TIME__ "]" ": ");\
	fprintf(stdout, "" __VA_ARGS__);\
	fprintf(stdout, "\n");\
	log_sync_unlock();\
	trap();\
}while(0)

#define err(...) errl(-, "" __VA_ARGS__)

#define verifyl(label, expr, ...)\
do\
{\
	if(!(expr))\
	{\
		errl(label {expr}, __VA_ARGS__);\
	}\
}while(0)

#define verify(expr, ...) verifyl(-, expr, __VA_ARGS__)

#ifdef jahoda_debug
#define dbg(...) __VA_ARGS__
#define dbg_verifyl(label, expr, ...) dbg(verifyl(label, expr, __VA_ARGS__))

#define dbg_verify(expr, ...) dbg_verifyl(-, expr, __VA_ARGS__)

#else 
#define dbg(...)
#define dbg_verify(expr, ...) (void)0
#define dbg_verifyl(label, expr, ...) (void)0
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define jahoda_alignof(type) __alignof__(type)
#else
	#error "alignof is not supported on this compiler"
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define jahoda_typeof(expr) __typeof__(expr)
#else
    #error "typeof is not supported on this compiler"
#endif

#define expand(x) x 

#define stringify(x) #x
#define xstringify(x) stringify(x)


#define stringify_case(value)\
case value:\
return #value

typedef struct
{
	u8 *data;
	uz len;
} memv;

#define memv_from(obj) (memv){.data = (u8*)(obj), .len = sizeof(*(obj))}
#define memv_fmt(mem) (i32)(mem)->len, (mem)->data
#define memv_dump_hex(mem) printf("{ "); for each_index(i, (mem)->len) { printf("%x ", (mem)->data[i]); } printf("}\n");
#define memv_as(mem, type) ((type*)((mem)->data))

typedef struct
{
	const char *data;
	uz len;
} strv;

strv strv_make(const char *data, uz len);
strv strv_from_cstr(const char *data);

bool8 strv_compare(strv strv1, strv strv2);

#define strv_fmt(view) (i32)(view)->len, (view)->data
#define cstrv(...) strv_from_cstr(__VA_ARGS__)

#endif	