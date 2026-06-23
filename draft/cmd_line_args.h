#ifndef draft_cmd_line_args
#define draft_cmd_line_args

// @impl_point
#define command_line_flag_args\
	X(h, "help")\

// @impl_point
#define command_line_data_args\
	X(endp, const_cstr, "0.0.0.0:7777", "server endpoint")

#include <jahoda/meta/cmd_line_args.h>

#endif