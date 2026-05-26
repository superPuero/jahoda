#include "window.h"
#include <assert.h>

window window_make_(window_params params)
{
	// if(!params.glfw_ctx->initialized)
	// {
	// 	fprintf(stderr, "glfw_context is not initialized");
	// 	return (window){0};
	// }

	char namebuf[window_name_max_len] = {0};
	memcpy(namebuf, params.title.data, params.title.len);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	
	// @explain: window is blank for few hundred milliseconds before everything else is initialized, and its looks ugly, 
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	
	window out = {
		.handle = glfwCreateWindow(params.width, params.height, namebuf, NULL, NULL), 
		.width = params.width, 
		.height = params.height, 
		.fullscreen = params.fullscreen
	};


	return out;
}

void window_make_visible(window* win)
{
	glfwShowWindow(win->handle);
}

void window_release(window* win)
{
	assert(win->handle);

    glfwDestroyWindow(win->handle);
}

bool8 window_should_close(window* win)
{
	assert(win->handle);

	return glfwWindowShouldClose(win->handle);
}

bool8 window_is_minimized(window* win)
{
    return glfwGetWindowAttrib(win->handle, GLFW_ICONIFIED) != 0;
}

bool8 window_mouse_button_down(window* win, int button)
{
	return glfwGetMouseButton(win->handle, button) == GLFW_PRESS;
}

vec2_f64 window_mouse_pos(window* win)
{
	vec2_f64 pos;
	glfwGetCursorPos(win->handle, &pos.x, &pos.y);
	return pos;
}

void window_poll_events(window* win)
{
	assert(win->handle);

	glfwPollEvents();
}
