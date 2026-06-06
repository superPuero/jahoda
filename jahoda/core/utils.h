#ifndef jahoda_utils
#define jahoda_utils

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <time.h>
#include "platform_detect.h"
#include "types.h"

#ifndef NDEBUG
#define jahoda_debug
#endif

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

#ifndef jahoda_disable_info
#define infol(label, ...)\
do\
{\
	print_colored_if_isatty(ansi_text_color_green, #label ": ");\
	fprintf(stdout, "" __VA_ARGS__);\
	fprintf(stdout, "\n");\
}while(0)
#define info(...) infol(info, "" __VA_ARGS__)
#else
#define infol(label, ...)
#define info(...)
#endif


#define warnl(label, ...)\
do\
{\
	print_colored_if_isatty(ansi_text_color_yellow, #label ": ");\
	fprintf(stdout, "" __VA_ARGS__);\
	fprintf(stdout, "\n");\
}while(0)

#define warn(...) warnl(warn, __VA_ARGS__);

#define errl(label, ...)\
do\
{\
	print_colored_if_isatty(ansi_text_color_red, #label ": ");\
	fprintf(stdout, "" __VA_ARGS__);\
	fprintf(stdout, "\n");\
	trap();\
}while(0)

#define err(...) errl(err, __VA_ARGS__)

#define verifyl(label, expr, ...)\
do\
{\
	if(!(expr))\
	{\
		errl(label, __VA_ARGS__);\
	}\
}while(0)

#define verify(expr, ...) verifyl(verify_fail, expr, __VA_ARGS__)

#ifdef jahoda_debug
#define dbg(...) __VA_ARGS__
#define dbg_verifyl(label, expr, ...)\
do\
{\
	if(!(expr))\
	{\
		errl(label, __VA_ARGS__);\
	}\
}while(0)

#define dbg_verify(expr, ...) dbg_verifyl(dbg_verify_fail, expr, __VA_ARGS__)

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
    #error "typeof is not supported on this compiler in pure C mode"
#endif

#define expand(x) x 

#define stringify(x) #x
#define xstringify(x) stringify(x)


#define stringify_case(value)\
case value:\
return #value

typedef struct
{
	const char *data;
	uz len;
} strv;

strv strv_make(const char *data, uz len);
strv strv_from_cstr(const char *data);

// @thoughs: why %.*s fmt takes signed integer?
// printing 2 gigabytes of data at once is not something that is reasonable anyways 
#define strv_fmt(view) (i32)(view)->len, (view)->data


#endif	