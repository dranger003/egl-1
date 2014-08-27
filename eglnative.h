#ifndef __EGLNATIVE_H__
#define __EGLNATIVE_H__

#include <EGL/egl.h>

EGLNativeDisplayType	egl_native_display_open();
void					egl_native_display_close(
							EGLNativeDisplayType display
						);

EGLNativeWindowType		egl_native_window_create(
							EGLNativeDisplayType display,
							const char *name,
							EGLint x,
							EGLint y,
							EGLint width,
							EGLint height,
							EGLint visual_id
						);
void					egl_native_window_destroy(
							EGLNativeDisplayType display,
							EGLNativeWindowType window
						);

void					egl_native_window_resize(
							EGLint width,
							EGLint height
						);
EGLBoolean				egl_native_window_process_events(
							EGLNativeDisplayType display,
							EGLNativeWindowType window
						);

#endif // __EGLNATIVE_H__
