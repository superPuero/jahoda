#ifndef jahoda_command_line_args
#define jahoda_command_line_args

#include <jahoda/core/core.h>

#define command_line_flag_args\
	XX(h, "help")


#define command_line_data_args\
	XXXX(ww, u32, 800, "window width")\
	XXXX(wh, u32, 800, "window height")    

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