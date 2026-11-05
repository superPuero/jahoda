#include <jahoda/core/core.h>

#ifndef cmd_line_args_base
#define cmd_line_args_base 
#include <stdlib.h>

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

static b8 b8_parse(char *str)
{
    if(strcmp(str, "true") == 0)
    {
        return true;
    }
    else if(strcmp(str, "false") == 0)
    {
        return false;        
    }
    else
    {
        err("wasnt able to parse b8 from %s", str);
    }
}

static cstr cstr_parse(char *str)
{    
    return str;
}

#endif

#define cmd_line_args_spec_decl(x) concat(cmd_line_args_spec, _##x)

typedef struct
{
	#define X(e, type, default, description) type e; b8 e##_provided;
	cmd_line_args
	#undef X

    #define X(e, ...) b8 e;
    cmd_line_flag_args
    #undef X

}  cmd_line_args_spec_decl(cmd_line_args);

static inline cmd_line_args_spec_decl(cmd_line_args) cmd_line_args_spec_decl(cmd_line_args_parse)(int argc, char **argv)
{
    cmd_line_args_spec_decl(cmd_line_args) opt = {0};

	for(i32 i = 1; i < argc; i++)
	{
		b8 found = false;

		#define X(e, type, default, description)\
		if(strcmp(argv[i], "-"#e) == 0)\
		{\
			opt.e = type##_parse(argv[i+1]);\
            opt.e##_provided = true;\
			found = true;\
            i++;\
		}
		cmd_line_args
		#undef X

        #define X(e, description)\
		if(strcmp(argv[i], "-"#e) == 0)\
		{\
			opt.e = true;\
			found = true;\
		}
		cmd_line_flag_args
		#undef X
		if(!found)
		{
            err("unrecognized argument %s\n", argv[i]);
            break;
		}
	}

    #define X(e, type, default, description)\
    if(!opt.e##_provided)\
    {\
        opt.e = default;\
    }
    cmd_line_args
    #undef X

	return opt;
}

static inline void cmd_line_args_spec_decl(cmd_line_args_display)(void)
{
    fprintf(stdout, "flags:\n");
    
    #define X(f, description) \
    fprintf(stdout, "-%-19s %s\n", #f, description);    
    cmd_line_flag_args
    #undef X

    fprintf(stdout, "\n");
    fprintf(stdout, "data args:\n");
    
    #define X(f, type, default_val, description) \
    fprintf(stdout, "-%-19s [%-5s | default: %-5s]   %s\n", \
            #f, #type, #default_val, description);    
    cmd_line_args
    #undef X

    fprintf(stdout, "\n");
}

#undef cmd_line_args_spec
#undef cmd_line_args