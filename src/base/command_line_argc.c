#include "command_line_args.h"	
#include <errno.h>

static u32 u32_parse(char *str)
{    
    char* endptr;
    errno = 0;
    
    unsigned long val = strtoul(str, &endptr, 10);
    
    verify(errno == 0, "unable to parse u32 from \"%s\" (overflow)", str);
    verify(endptr != str, "unable to parse u32 from \"%s\" (no digits found)", str);
    verify(*endptr == '\0', "unable to parse u32 from \"%s\" (invalid characters)", str);
    verify(val <= UINT32_MAX, "unable to parse u32 from \"%s\" (exceeds 32-bit limit)", str);

    return val;
}

command_line_args command_line_args_parse(int argc, char **argv)
{
	command_line_args opt = {0};

	for(i32 i = 1; i < argc; i++)
	{
		bool8 found = false;

		#define XXXX(e, type, default, ...)\
		if(strcmp(argv[i], "-"#e) == 0)\
		{\
			opt.e = type##_parse(argv[i+1]);\
            opt.e##_provided = true;\
			found = true;\
            i++;\
		}
		command_line_data_args
		#undef XXXX

        #define XX(e, ...)\
		if(strcmp(argv[i], "-"#e) == 0)\
		{\
			opt.e = true;\
			found = true;\
		}
		command_line_flag_args
		#undef XX
		if(!found)
		{
            errl(command_line_args, "unrecognized argument %s\n", argv[i]);
            break;
		}
	}

    #define XXXX(e, type, default, ...)\
    if(!opt.e##_provided)\
    {\
        opt.e = default;\
    }
    command_line_data_args
    #undef XXXX

	return opt;
}

void command_line_args_display(void)
{
    fprintf(stdout, "flags:\n");
    
    #define XX(f, ...) \
    fprintf(stdout, "-%-19s %s\n", #f, __VA_ARGS__);    
    command_line_flag_args
    #undef XX

    fprintf(stdout, "\n");
    fprintf(stdout, "data arguments:\n");
    
    #define XXXX(f, type, default_val, ...) \
    fprintf(stdout, "-%-19s [%-5s | default: %-5s]   %s\n", \
            #f, #type, #default_val, __VA_ARGS__);    
    command_line_data_args
    #undef XXXX

    fprintf(stdout, "\n");
}
