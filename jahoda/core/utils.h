#ifndef jahoda_utils
#define jahoda_utils

#include <stdio.h>
#include <stdarg.h>
#include "platform_detect.h"
#include "types.h"

#if defined(__GNUC__) || defined(__clang__)
#else
	#error "compiler is not supported"
#endif

#define auto __auto_type
#define per_thread __thread
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
#define has_bit(mask, b) (mask & bit(b))

#ifndef NDEBUG
#define jahoda_debug
#define is_debug 1
#endif

#ifndef is_debug
#define is_debug 0
#endif

#define ref [const static 1]

#define concat_helper(x, y) x##y
#define concat(x, y) concat_helper(x,y)

#define each_index_range(index_var_name, from, to)\
(u64 index_var_name = (from); index_var_name < (to); index_var_name++)

#define each_index_range_signed(index_var_name, from, to)\
(i64 index_var_name = from; index_var_name < to; index_var_name++)

#define each_index(index_var_name, count) each_index_range(index_var_name, 0, count)

#define unique_symbol(...) __VA_ARGS__##__COUNTER__	

#define static_assert(expr) typedef int static_assert_##__COUNTER__[(expr) ? 1 : -1]

#define arrsize(...) (sizeof(__VA_ARGS__)/sizeof(__VA_ARGS__[0]))

#define ansi_esq_seq "\033"

#define ansi_text_color_green "32"
#define ansi_text_color_red "31"
#define ansi_text_color_yellow "33"
#define ansi_text_color_blue "34"
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

void print_colored_if_isatty(const char* color, const char* str);

void log_dest(FILE *file);
FILE *get_log_dest();

#if 0
#define msgl(label, ...)({\
	log_sync_lock();\
	print_colored_if_isatty(ansi_text_color_blue, #label ": ");\
	fprintf(get_log_dest(), "" __VA_ARGS__);\
	fprintf(get_log_dest(), "\n");\
	log_sync_unlock();\
})
#define infol(label, ...)({\
	log_sync_lock();\
	print_colored_if_isatty(ansi_text_color_green, #label ": ");\
	fprintf(get_log_dest(), "" __VA_ARGS__);\
	fprintf(get_log_dest(), "\n");\
	log_sync_unlock();\
})
#define warnl(label, ...)({\
	log_sync_lock();\
	print_colored_if_isatty(ansi_text_color_yellow, #label ": ");\
	fprintf(get_log_dest(), "" __VA_ARGS__);\
	fprintf(get_log_dest(), "\n");\
	log_sync_unlock();\
})
#define errl(label, ...)({\
	log_sync_lock();\
	print_colored_if_isatty(ansi_text_color_red, #label ": ");\
	fprintf(get_log_dest(), "" __VA_ARGS__);\
	fprintf(get_log_dest(), "\n");\
	log_sync_unlock();\
	trap();\
})
#else
#define msgl(label, ...) (void)(0)
#define infol(label, ...) (void)(0)
#define warnl(label, ...) (void)(0)
#define errl(label, ...) (void)(0)
#endif

#define msg(...) msgl(msg, __VA_ARGS__)
#define info(...) infol(info, __VA_ARGS__)
#define warn(...) warnl(warn, __VA_ARGS__)
#define err(...) errl(err, __VA_ARGS__)

#define verifyl(label, expr, ...)\
do\
{\
	if(!(expr))\
	{\
		errl(label {expr}, __VA_ARGS__);\
	}\
}while(0)

#define verify(expr, ...) verifyl(-, expr, __VA_ARGS__)

#define verifywl(label, expr, ...)\
do\
{\
	if(!(expr))\
	{\
		warnl(label {expr}, __VA_ARGS__);\
	}\
}while(0)

#define verifyw(expr, ...) verifywl(-, expr, __VA_ARGS__)

#ifdef jahoda_debug
#define dbg(...) __VA_ARGS__
#define dbg_verifyl(label, expr, ...) dbg(verifyl(label, expr, __VA_ARGS__))

#define dbg_verify(expr, ...) dbg_verifyl(-, expr, __VA_ARGS__)

#else 
#define dbg(...)
#define dbg_verify(expr, ...) (void)0
#define dbg_verifyl(label, expr, ...) (void)0
#endif

#define alignas(v) __attribute__((aligned(v)))
#define nodiscard __attribute__((warn_unused_result)) 

#define alignof(...) __alignof__(__VA_ARGS__)
#define typeof(...) __typeof__(__VA_ARGS__)

#define likely(...)   __builtin_expect(!!(__VA_ARGS__), 1)
#define unlikely(...) __builtin_expect(!!(__VA_ARGS__), 0)

#define unreachable() __builtin_unreachable()

#define expand(x) x

#define stringify(x) #x
#define xstringify(x) stringify(x)

#define stringify_case(value)\
case value:\
return #value

typedef struct
{
	u8 *data;
	uz size;
} memv;

b8 is_whitespace(u8 ch);

#define text_declare(typename, size)\
typedef struct\
{\
    char data[size];\
} typename;\
static inline typename typename##_make_va(const char *restrict fmt, va_list list)\
{\
	typename out = {0};\
	va_list list_copy;\
	va_copy(list_copy, list);\
	int res = vsnprintf(out.data, sizeof(typename), fmt, list_copy);\
	dbg(\
		if((uz)res >= sizeof(typename))\
		{\
			warnl(typename##_make, "truncated: (%s) capacity (%zu) got (%i)", out.data, sizeof(typename), res + 1);\
		}\
	)\
	va_end(list_copy);\
	return out;\
}\
static inline typename typename##_make(const char *restrict fmt, ...)\
{\
	typename out = {0};\
    va_list list;\
    va_start(list, fmt);\
    out = typename##_make_va(fmt, list);\
    va_end(list);\
	return out;\
}\
static inline b8 typename##_cmp(typename *v1, typename *v2)\
{\
	return memcmp(v1->data, v2->data, sizeof(typename)) == 0;\
}\
static inline b8 typename##_cmp_fmt(typename *v, const char *restrict fmt, ...)\
{\
    typename out = {0};\
	va_list list;\
    va_start(list, fmt);\
    out = typename##_make_va(fmt, list);\
	return memcmp(out.data, v->data, sizeof(typename)) == 0;\
}

typedef struct
{
	u32 size;
	u32 alignment;
} type_info;

#define bitmask_make_1(c) bit(c)
#define bitmask_make_2(c1, c2) bit(c1) | bit(c2)
#define bitmask_make_3(c, ...) bitmask_make_2(__VA_ARGS__) | bit(c)
#define bitmask_make_4(c, ...) bitmask_make_3(__VA_ARGS__) | bit(c)
#define bitmask_make_5(c, ...) bitmask_make_4(__VA_ARGS__) | bit(c)
#define bitmask_make_6(c, ...) bitmask_make_5(__VA_ARGS__) | bit(c)
#define bitmask_make_7(c, ...) bitmask_make_6(__VA_ARGS__) | bit(c)
#define bitmask_make_8(c, ...) bitmask_make_7(__VA_ARGS__) | bit(c)

#define bitmask_make_router(_1, _2, _3, _4, _5, _6, _7, _8, func, ...) func
#define bitmask_make(...) (bitmask_make_router(__VA_ARGS__, bitmask_make_8, bitmask_make_7, bitmask_make_6, bitmask_make_5, bitmask_make_4, bitmask_make_3, bitmask_make_2, bitmask_make_1)(__VA_ARGS__))


#define type_info_of(type) (type_info){.size = (u32)sizeof(type), .alignment = (u32)alignof(type)}
#define text_fmt(text) (text)->data

text_declare(text8, 8);
text_declare(text16, 16);
text_declare(text32, 32);
text_declare(text64, 64);
text_declare(text128, 128);
text_declare(text256, 256);

#endif	