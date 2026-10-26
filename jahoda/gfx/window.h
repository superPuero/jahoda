#ifndef jahoda_window
#define jahoda_window

#include "glfw_context.h" 
#include <jahoda/core/core.h>
#include <jahoda/core/math.h>

#define window_name_max_len 256

typedef struct window window;

typedef void (window_key_callback)(window *win, void* arg, int key, int scancode, int action, int mods);

struct window
{
	GLFWwindow *handle;
	ivec2 _last_pos;
	ivec2 _last_size;
	b8 fullscreen;	
	b8 keys_down[GLFW_KEY_LAST];
	b8 keys_pressed[GLFW_KEY_LAST];
	b8 keys_just_pressed[GLFW_KEY_LAST];
	struct {
		window_key_callback *key_fn;
		void *arg;
	} callback;
};


typedef struct
{
	arena *mem;
	ivec2 pos;
	ivec2 size;
	uz vsync;
	b8 fullscreen;
	text64 title;
	b8 invisible;
	struct {
		window_key_callback *key_fn;
		void *arg;
	} callback;
} window_params;

#define window_make(...) window_make_((window_params){__VA_ARGS__})

window *window_make_(window_params params);
void window_release(window *win);
b8 window_should_close(window *win);
b8 window_is_minimized(window *win);
b8 window_mouse_button_down(window *win, int button);
b8 window_key_just_pressed(window *win, int button);
b8 window_key_down(window *win, int button);
vec2_f32 window_mouse_pos(window *win);
void window_poll_events(window *win);
void window_make_visible(window *win);
void window_wait_events(window *win);
void window_toggle_fullscreen(window *win);
#endif