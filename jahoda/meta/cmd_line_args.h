#ifndef jahoda_cmd_line_args
#define jahoda_cmd_line_args

#include <jahoda/core/core.h>

typedef const char *const_cstr;

typedef struct
{
	#define X(e, type, default, ...) type e; bool8 e##_provided;
	command_line_data_args
	#undef X

    #define X(e, ...) bool8 e;
    command_line_flag_args
    #undef X

} cmd_line_args;

cmd_line_args cmd_line_args_parse(int argc, char **argv);
void cmd_line_args_display(void);

#ifdef jahoda_cmd_line_args_impl

static u32 u32_parse(char *str)
{    
    char *endptr;
    errno = 0;
    
    u32 val = strtoul(str, &endptr, 10);
      
    verify(errno == 0, "unable to parse u32 from \"%s\" (overflow)", str);
    verify(endptr != str, "unable to parse u32 from \"%s\" (no digits found)", str);
    verify(*endptr == '\0', "unable to parse u32 from \"%s\" (invalid characters)", str);

    return val;
}

static f64 f64_parse(char *str)
{    
    char *endptr;
    errno = 0;
    
    f64 val = strtod(str, &endptr);
    
    verify(errno == 0, "unable to parse f64 from \"%s\" (overflow)", str);
    verify(endptr != str, "unable to parse f64 from \"%s\" (no digits found)", str);
    verify(*endptr == '\0', "unable to parse f64 from \"%s\" (invalid characters)", str);

    return val;
}

static const_cstr const_cstr_parse(char *str)
{    
    return str;
}

cmd_line_args cmd_line_args_parse(int argc, char **argv)
{
	cmd_line_args opt = {0};

	for(i32 i = 1; i < argc; i++)
	{
		bool8 found = false;

		#define X(e, type, default, ...)\
		if(strcmp(argv[i], "-"#e) == 0)\
		{\
			opt.e = type##_parse(argv[i+1]);\
            opt.e##_provided = true;\
			found = true;\
            i++;\
		}
		command_line_data_args
		#undef X

        #define X(e, ...)\
		if(strcmp(argv[i], "-"#e) == 0)\
		{\
			opt.e = true;\
			found = true;\
		}
		command_line_flag_args
		#undef X
		if(!found)
		{
            errl(cmd_line_args, "unrecognized argument %s\n", argv[i]);
            break;
		}
	}

    #define X(e, type, default, ...)\
    if(!opt.e##_provided)\
    {\
        opt.e = default;\
    }
    command_line_data_args
    #undef X

	return opt;
}

void cmd_line_args_display(void)
{
    fprintf(stdout, "flags:\n");
    
    #define X(f, ...) \
    fprintf(stdout, "-%-19s %s\n", #f, __VA_ARGS__);    
    command_line_flag_args
    #undef X

    fprintf(stdout, "\n");
    fprintf(stdout, "data arguments:\n");
    
    #define X(f, type, default_val, ...) \
    fprintf(stdout, "-%-19s [%-5s | default: %-5s]   %s\n", \
            #f, #type, #default_val, __VA_ARGS__);    
    command_line_data_args
    #undef X

    fprintf(stdout, "\n");
}
#endif

#endif