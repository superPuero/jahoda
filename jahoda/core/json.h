#ifndef jahoda_json
#define jahoda_json

#include <jahoda/core/file.h>
#include <jahoda/core/arena.h>

typedef enum
{
    json_type_none = 0,
    json_type_null,
    json_type_object,
    json_type_array,
    json_type_string,
    json_type_u64,
    json_type_i64,
    json_type_f64,  
    json_type_bool        
} json_type;

struct json_value_s;
typedef struct json_value_s json_value;
typedef json_value json;

struct json_object_member_s;
typedef struct json_object_member_s json_object_member;

da_declare(json_value, json_value_da);
da_declare(json_object_member, json_object_member_da);

typedef json_value_da json_array;

typedef struct
{
    json_object_member_da members;
} json_object;

struct json_value_s
{
    json_type type;
    union
    {
        json_object object;
        json_array array;
        str string;
        i64 int64;
        u64 uint64;
        f64 float64;
        bool8 boolean;
    } content;
};

struct json_object_member_s
{
    json_value name;
    json_value value;
};

typedef struct
{
    arena static_mem;
    arena temp_mem;
    u32 current;
    strv view;
    bool8 done;
    struct 
    {
        bool8 happened;
        str what;
        u32 where;
        u32 line;
        const char *func;
    } err; 
} json_parser;

json json_from_strv(arena static_mem, arena temp_mem, strv view);
void json_parse(json *out, json_parser *parser);
void json_dump(json *j);
void json_parse_string(json *out, json_parser *parser);
json_object json_get_object(json *json);
json_array json_get_array(json *json);
json json_get_array_item(json *json, u32 index);
u64 json_get_u64(json *json);
f64 json_get_f64(json *json);
void json_parser_advance(json_parser *parser);
#endif