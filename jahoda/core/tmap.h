#ifndef jahoda_tmap
#define jahoda_tmap

#include <jahoda/core/utils.h>
#include <jahoda/core/hash.h>

#define tmap_declare(text_type, value_type, capacity, typename)\
typedef struct\
{\
    text_type ident[capacity];\
    value_type value[capacity];\
    text_type temp;\
    u64 occupied;\
} typename;\
value_type *typename##_insert(typename *map, const char *restrict fmt, ...)\
{\
    va_list list;\
    va_start(list, fmt);\
    map->temp = text_type##_make_va(fmt, list);\
    va_end(list);\
    dbg(\
        for each_index(i, map->occupied)\
        {\
            if(text_type##_cmp(&map->temp, map->ident + i))\
            {\
                errl(typename##_insert, "[%s] collision while inserting [\"%s\"]", #typename, map->temp.data);\
            }\
        }\
    )\
    map->ident[map->occupied] = map->temp;\
    dbg_verifyl(typename##_insert, map->occupied != arrlen(map->value), "[%s] overflow while inserting [\"%s\"]", #typename, map->temp.data);\
    return map->value + map->occupied++;\
}\
value_type *typename##_get(typename *map, const char *restrict fmt, ...)\
{\
    va_list list;\
    va_start(list, fmt);\
    map->temp = text_type##_make_va(fmt, list);\
    va_end(list);\
    for each_index(i, map->occupied)\
    {\
        if(text_type##_cmp(&map->temp, map->ident + i))\
        {\
            return map->value + i;\
        }\
    }\
    return NULL;\
}\
void typename##_remove(typename *map, const char *restrict fmt, ...)\
{\
    va_list list;\
    va_start(list, fmt);\
    map->temp = text_type##_make_va(fmt, list);\
    va_end(list);\
    for each_index(i, map->occupied)\
    {\
        if(text_type##_cmp(&map->temp, map->ident + i))\
        {\
            map->ident[i] = map->ident[--map->occupied];\
            map->value[i] = map->value[map->occupied];\
            return;\
        }\
    }\
}

#endif