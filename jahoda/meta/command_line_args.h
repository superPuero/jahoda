#ifndef jahoda_command_line_args
#define jahoda_command_line_args

#include <jahoda/core/core.h>

typedef const char *const_cstr;

// @impl_point
#define command_line_flag_args\
	XX(h, "help")\
	XX(preload, "preload model.bin")


// @impl_point
#define command_line_data_args\
	XXXX(lr, f64, 0.001, "model learning rate")\
	XXXX(ww, u32, 800, "window width")\
	XXXX(wh, u32, 800, "window height")\
	XXXX(wn, const_cstr, "default_name", "window name")

typedef struct
{
	#define XXXX(e, type, default, ...) type e; bool8 e##_provided;
	command_line_data_args
	#undef XXXX

    #define XX(e, ...) bool8 e;
    command_line_flag_args
    #undef XX

} command_line_args;

command_line_args command_line_args_parse(int argc, char **argv);
void command_line_args_display(void);

#endif