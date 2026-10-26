#include "window.h"
#include "gpu_glfw.h"

void glfw_kb_callback(GLFWwindow* handle, int key, int scancode, int action, int mods)
{
	window *win = (window*)glfwGetWindowUserPointer(handle);
	win->callback.key_fn(win, win->callback.arg, key, scancode, action, mods);
}

window *window_make_(window_params params)
{	
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	GLFWwindow *handle = NULL;

	if(params.fullscreen)
	{
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);		

		handle = glfwCreateWindow(mode->width, mode->height, params.title.data, monitor, NULL);		
	}
	else {
		handle = glfwCreateWindow(params.size.x, params.size.y, params.title.data, NULL, NULL);
		glfwSetWindowMonitor(handle, NULL, params.pos.x, params.pos.y, params.size.x, params.size.y, 0);
	}

	
	if(params.invisible)
	{
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	}
	
	if (glfwRawMouseMotionSupported()) 
	{
        glfwSetInputMode(handle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
	else 
	{
		warnl(glfw, "raw mouse motion is not supported");	
	}

	window *out = arena_push(
		params.mem, 
		(window){
			.handle = handle,
			._last_pos = params.pos,
			._last_size = params.size, 
			.fullscreen = params.fullscreen,
			.callback = { 
				.key_fn = params.callback.key_fn,
				.arg = params.callback.arg,
			}
		}
	);
	
	if(params.callback.key_fn)
	{
		glfwSetKeyCallback(out->handle, glfw_kb_callback);
		glfwSetWindowUserPointer(out->handle, out);
	}

	return out;
}

void window_toggle_fullscreen(window *win)
{
	if(win->fullscreen)
	{						
		glfwSetWindowMonitor(
			win->handle,
			NULL, 
			win->_last_pos.x, win->_last_pos.y, 
			win->_last_size.x, win->_last_size.y, 
			0
		);
		
		win->fullscreen = false;
	}
	else 
	{	
		glfwGetWindowSize(win->handle, &win->_last_size.x, &win->_last_size.y);
		glfwGetWindowPos(win->handle, &win->_last_pos.x, &win->_last_pos.y);
	
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		
		glfwSetWindowMonitor(
			win->handle, 
			monitor, 
			0,0 , 
			mode->width, mode->height, 
			mode->refreshRate
		);

		win->fullscreen = true;
	}
}

void window_make_visible(window *win)
{
	glfwShowWindow(win->handle);
}

void window_release(window *win)
{
	dbg_verify(win->handle);

    glfwDestroyWindow(win->handle);
}

b8 window_should_close(window *win)
{
	dbg_verify(win->handle);

	return glfwWindowShouldClose(win->handle);
}

b8 window_is_minimized(window *win)
{
    return glfwGetWindowAttrib(win->handle, GLFW_ICONIFIED) != 0;
}

b8 window_mouse_button_down(window *win, int button)
{
	return glfwGetMouseButton(win->handle, button) == GLFW_PRESS;
}

b8 window_key_just_pressed(window *win, int button)
{
	return win->keys_pressed[button];
}

b8 window_key_down(window *win, int button)
{
	return glfwGetKey(win->handle, button) == GLFW_PRESS;
}

vec2_f32 window_mouse_pos(window *win)
{
	vec2_f64 proxy;
	glfwGetCursorPos(win->handle, &proxy.x, &proxy.y);
	return (vec2_f32){proxy.x, proxy.y};
}

void window_poll_events(window *win)
{
	dbg_verify(win->handle);

	glfwPollEvents();
}

void window_wait_events(window *win)
{	
	glfwWaitEvents();
}
