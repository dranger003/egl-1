#include "eglnative.h"

#include <GLES2/gl2.h>

EGLNativeDisplayType egl_native_display_open()
{
	return (EGLNativeDisplayType)XOpenDisplay(0);
}

void egl_native_display_close(EGLNativeDisplayType display)
{
	XCloseDisplay((Display *)display);
}

EGLNativeWindowType egl_native_window_create(
	EGLNativeDisplayType display,
	const char *name,
	EGLint x,
	EGLint y,
	EGLint width,
	EGLint height,
	EGLint visual_id)
{
	Display *_display = (Display *)display;

	int screen = XDefaultScreen(_display);
	Window root_window = XRootWindow(_display, screen);

	XVisualInfo xvi_req =
	{
		.visualid = visual_id,
	};
	int n;
	XVisualInfo *xvi_res = XGetVisualInfo(_display, VisualIDMask, &xvi_req, &n);
	Visual *visual;
	if (xvi_res)
	 	visual = xvi_res->visual;
	else
		visual = XDefaultVisual(_display, screen);

	Colormap colormap = XCreateColormap(_display, root_window, visual, AllocNone);

	XSetWindowAttributes xswa =
	{
		.background_pixel = 0,
		.border_pixel = 0,
		.colormap = colormap,
		.event_mask =
			ExposureMask |
			FocusChangeMask |
			KeyPressMask |
			KeyReleaseMask |
			ButtonPressMask |
			ButtonReleaseMask |
			PointerMotionMask |
			StructureNotifyMask,
	};

	Window window = XCreateWindow(
		display,
		root_window,
		x,
		y,
		width,
		height,
		0,
		XDefaultDepth(_display, screen),
		InputOutput,
		visual,
		CWBackPixel | CWBorderPixel | CWColormap | CWEventMask,
		&xswa
	);

	Atom atom = XInternAtom(_display, "WM_DELETE_WINDOW", True);
	XSetWMProtocols(_display, window, &atom, 1);

	XSizeHints hints =
	{
		.flags = USSize | USPosition,
		.x = x,
		.y = y,
		.width = width,
		.height = height,
	};
	XSetStandardProperties(
		_display,
		window,
		name,
		name,
		None,
		(char **)0,
		0,
		&hints
	);

	XMapWindow(_display, window);

	return (EGLNativeWindowType)window;
}

void egl_native_window_destroy(EGLNativeDisplayType display, EGLNativeWindowType window)
{
	Display *_display = (Display *)display;
	Window _window = (Window)window;

	XWindowAttributes xwa;
	XGetWindowAttributes(_display, _window, &xwa);

	XUnmapWindow(_display, _window);
	XDestroyWindow(_display, _window);
	XFreeColormap(_display, xwa.colormap);
}

void egl_native_window_resize(EGLint width, EGLint height)
{
	glViewport(0, 0, width, height);
}

EGLBoolean egl_native_window_process_events(EGLNativeDisplayType display, EGLNativeWindowType window)
{
	Display *_display = (Display *)display;
	Window _window = (Window)window;

	while (XEventsQueued(_display, QueuedAfterFlush))
	{
		XEvent evt;
		XNextEvent(_display, &evt);

		switch (evt.type)
		{
			case ClientMessage:
				if (evt.xmotion.window == _window)
					if (evt.xclient.data.l[0] == XInternAtom(_display, "WM_DELETE_WINDOW", True))
						return EGL_FALSE;
				break;
			case ConfigureNotify:
				if (evt.xconfigure.window == _window)
					egl_native_window_resize(evt.xconfigure.width, evt.xconfigure.height);
				break;
/*
			case MotionNotify:
				if (evt.xmotion.window == _window)
					// evt.xmotion.x, evt.xmotion.y
					// evt.xmotion.state & Button1Mask
				break;
*/
			default:
				break;
		}
	}

	return EGL_TRUE;
}
