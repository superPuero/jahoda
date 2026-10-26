#ifndef cmd_line_args
#define cmd_line_args

#define cmd_line_max_args 64

#include "tmap.h"

typedef enum 
{
    command_line_args_content_type_b8,
    command_line_args_content_type_u32,  
    command_line_args_content_type_f32,  
} command_line_args_content_type;

typedef struct
{
    command_line_args_content_type type;

    union
    {
        b8 b8;
        u32 u32;
        f32 f32;
    };
} command_line_args_content;

typedef struct
{
    text64 name;
    command_line_args_content_type type;
} command_line_args_parse_entry;

typedef struct
{
    u32 entries_count;
    command_line_args_parse_entry entries[cmd_line_max_args];
} command_line_args_parse_params;

typedef struct
{
    text64 name;
    command_line_args_content content;
} command_line_args_parsed_entry;

typedef struct
{
    u32 entries_count;
    command_line_args_parsed_entry entries[cmd_line_max_args];
} command_line_args_parsed;

command_line_args_parsed command_line_args_parse_(command_line_args_parse_params params);

#define command_line_arg(name_, type_) name = text64_make(name_), .type = command_line_args_content_type_##type_

#define command_line_args_parse(...) command_line_args_parse_((command_line_args_parse_params){ .entries_count = arrsize((command_line_args_parse_entry[]){__VA_ARGS__}), .entries = (command_line_args_parse_entry[]){__VA_ARGS__}})

#endif 