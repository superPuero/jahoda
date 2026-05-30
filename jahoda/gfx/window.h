#ifndef jahoda_window
#define jahoda_window

#include "glfw_context.h" 
#include <jahoda/core/core.h>
#include <jahoda/core/math.h>


#define window_name_max_len 256

typedef struct
{
	GLFWwindow *handle;
	uz width;
	uz height;
	bool8 fullscreen;
} window;

typedef struct
{
	uz width;
	uz height;
	uz vsync;
	bool8 fullscreen;
	strv title;
} window_params;

#define window_make(...) window_make_((window_params){__VA_ARGS__});

window window_make_(window_params params);
void window_release(window *win);
bool8 window_should_close(window *win);
bool8 window_is_minimized(window *win);
bool8 window_mouse_button_down(window *win, int button);
vec2_f64 window_mouse_pos(window *win);
void window_poll_events(window *win);
void window_make_visible(window *win);
#endif