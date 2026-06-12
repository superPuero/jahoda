#include "json.h"

#include <ctype.h>
#include "json.h"

#define current(parser) (parser)->view.data[(parser)->current]
#define cc(parser, ch) case ch: json_parser_advance(parser);
#define json_parser_error_and_return(parser, ...) (parser)->done = true; (parser)->err.happened = true; (parser)->err.what = str_from_fmt((parser)->temp_mem, __VA_ARGS__); (parser)->err.where = (parser)->current; (parser)->err.func = __func__; (parser)->err.line = __LINE__; return;
#define json_parser_advance(parser)\
do\
{\
    if((parser)->done)\
    {\
        json_parser_error_and_return((parser), "unexpected json end");\
    }\
    (parser)->current++;\
    if(parser->current == (parser)->view.len)\
    {\
        (parser)->done = true;\
    }\
}while(0)

static void json_skip_whitespace(json_parser *parser)
{
    while (parser->current < parser->view.len)
    {
        char c = current(parser);
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t' )
        {
            json_parser_advance(parser);
        }
        else
        {
            break;
        }
    }
}

void json_parser_expect(json_parser *parser, char ch)
{
    if(parser->view.data[parser->current] != ch)
    {
        json_parser_error_and_return(parser, "unexpected %c expected %c", parser->view.data[parser->current], ch);
    }
}

json json_from_strv(arena static_mem, arena temp_mem, strv view)
{ 
    json out = {0};
    
    json_parser parser = { .static_mem = static_mem, .temp_mem = temp_mem, .view = view };

    scratch static_scratch = scratch_begin(parser.static_mem);
    scratch temp_scratch = scratch_begin(parser.temp_mem);
    
    json_parse(&out, &parser);
    
    if(parser.err.happened)
    {
        dbg(warnl(json_parse, "%.*s at char %u, at %s, on line %u", str_fmt(&parser.err.what), parser.err.where, parser.err.func, parser.err.line));
        scratch_end(static_scratch);
        out.type = json_type_none;
    }
    
    scratch_end(temp_scratch);
    
    return out;
}

void json_populate(json *out, json_type type)
{
    *out = (json){ .type = type };
}

void json_parse_array(json *out, json_parser *parser)
{
    json_populate(out, json_type_array);
    json_parser_advance(parser);   
    json_skip_whitespace(parser);
    
    u8 c = current(parser);

    while(true)
    {
        switch(c)
        {
            case ']':
                json_parser_advance(parser);
                return;

            case ',': 
                if(out->content.array.occupied == 0)
                {
                    json_parser_error_and_return(parser, "unexpected comma");
                }                
                json_parser_advance(parser);
                json_skip_whitespace(parser);
                da_append(parser->static_mem, &out->content.array, (json){0});
                json_parse(da_last(&out->content.array), parser);
                break;

            default:
                if(out->content.array.occupied == 0)
                {
                    da_append(parser->static_mem, &out->content.array, (json){0});
                    json_parse(da_last(&out->content.array), parser);                
                }
                else
                {
                    json_parser_error_and_return(parser, "expected comma or array terminator ']'");
                }
        }
                        
        json_skip_whitespace(parser);
        c = current(parser);

        if(parser->err.happened)
        {
            break;
        }
    }


}

void json_parse_object(json *out, json_parser *parser)
{
    json_parser_advance(parser);

    json_populate(out, json_type_object);

    json_skip_whitespace(parser);
    u8 c = current(parser);

    while(true)
    {
        switch(c)
        {
            case '}':
                json_parser_advance(parser);
                return;

            case ',':        
                if(out->content.object.members.occupied == 0)
                {
                    json_parser_error_and_return(parser, "unexpected comma");                
                }                        
                json_parser_advance(parser);
                json_skip_whitespace(parser);
                json_parser_expect(parser, '"');
            case '"':
                da_append(parser->static_mem, &out->content.object.members, (json_object_member){});
                json_parse_string(&da_last(&out->content.object.members)->name, parser);
                json_skip_whitespace(parser);
                json_parser_expect(parser, ':');
                json_parser_advance(parser);
                json_parse(&da_last(&out->content.object.members)->value, parser);
                break;

            default:
                json_parser_error_and_return(parser, "expected object member, comma or object terminator '}'");
        }
                        
        json_skip_whitespace(parser);
        c = current(parser);

        if(parser->err.happened)
        {
            break;
        }
    }

}


void json_parse_number(json *out, json_parser *parser)
{
    u32 start = parser->current;
    
    json_parser_advance(parser);        

    u8 c = current(parser);

    bool8 dot_found = false;

    while((isdigit(c) || c == '.') && !parser->done)
    {
        if(c == '.')
        {
            if(dot_found)
            {
                json_parser_error_and_return(parser, "undexpected double dot");
            }
            dot_found = true;
        }        

        json_parser_advance(parser);        

        c = current(parser);
    }

    // @mem: add scratch    
    str number_str = str_from_view_nt(parser->temp_mem, (strv){.data = parser->view.data + start, .len = parser->current - start});

    if(dot_found)
    {
        char* end;
        json_populate(out, json_type_f64);
        out->content.float64 = strtod(number_str.data, &end);
    }
    else if(number_str.data[0] == '-')
    {
        char* end;
        json_populate(out, json_type_i64);
        out->content.int64 = strtoll(number_str.data, &end, 10);
    }
    else
    {
        char* end;
        json_populate(out, json_type_u64);
        out->content.uint64 = strtoull(number_str.data, &end, 10);
    }
}

void json_dump(json *j)
{
    switch(j->type)
    {
         case json_type_array:        
            fprintf(stdout, "[ ");
            for da_each(&j->content.array, it)
            {
                if(it != j->content.array.data)
                {
                    fprintf(stdout, ", ");
                }

                json_dump(it);
            }
            fprintf(stdout, " ]");
            break;
        case json_type_object:        
            fprintf(stdout, "{ ");
            for da_each(&j->content.object.members, it)
            {
                if(it != j->content.object.members.data)
                {
                    fprintf(stdout, ", ");
                }

                json_dump(&it->name);
                fprintf(stdout, ": ");
                json_dump(&it->value);
            }
            fprintf(stdout, " }");
            break;

        case json_type_i64:
            fprintf(stdout, "%lli", j->content.int64);
            break;
         case json_type_f64:
            fprintf(stdout, "%lf", j->content.float64);
            break;
        case json_type_u64:
            fprintf(stdout, "%llu", j->content.uint64);
            break;
        case json_type_string:
            fprintf(stdout, "\"%.*s\"", str_fmt(&j->content.string));
            break;
    }
}

void json_parse_string(json *out, json_parser *parser)
{
    json_parser_advance(parser);        
    json_populate(out, json_type_string);
    
    u8 c = current(parser);

    bool8 escaped = false;

    while(escaped || c != '"')
    {
        if(escaped)
        {
            switch(c)
            {
                case 'u':
                    json_parser_error_and_return(parser, "unicode escape sequences are currently unsupported %c", c);     
                case '"':
                    c = '"';
                    break;
                case '\\':
                    c = '\\';
                    break;
                case '/':
                    c = '/';
                    break;
                case 'b':
                    c = '\b';
                    break;
                case 'f':
                    c = '\f';
                    break;
                case 'n':
                    c = '\n';
                    break;
                case 't':
                    c = '\t';
                    break;
                case 'r':
                    c = '\r';
                    break;
                default:  
                    json_parser_error_and_return(parser, "invalid escape character %c", c);                                
            }
            da_append(parser->static_mem, &out->content.string, c);
            
            escaped = false;
        }
        else if (c == '\\')
        {
            escaped = true;
        }
        else 
        {
            da_append(parser->static_mem, &out->content.string, c);
        }

        json_parser_advance(parser);        
        c = current(parser);
    }

    
    json_parser_advance(parser);
}



void json_parse(json *out, json_parser *parser)
{    
    json_skip_whitespace(parser);

    switch(current(parser))
    {
        case '"':
            json_parse_string(out, parser);
            break;
        case '[':
            json_parse_array(out, parser);
            break;
        case '{':
            json_parse_object(out, parser);
            break;
            case '-':
            case '0':
            case '1':  
            case '2':  
            case '3':  
            case '4':  
            case '5':  
            case '6':  
            case '7':  
            case '8':  
            case '9': 
            json_parse_number(out, parser); 
            break;
        }
}

