#include <stdio.h>
#include <assert.h>
#include <time.h>

#include <X11/Xutil.h>
#include "eglnative.h"

#include <GLES2/gl2.h>

#define NSEC	1000000000.0L
#define FPS		60.0L

struct egl_ctx
{
	EGLNativeDisplayType native_display;
	EGLNativeWindowType native_window;

	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;

	EGLint width;
	EGLint height;
};

EGLint egl_init(struct egl_ctx *ctx)
{
	ctx->native_display = egl_native_display_open();
	assert(ctx->native_display);

	ctx->display = eglGetDisplay(ctx->native_display);
	assert(ctx->display != EGL_NO_DISPLAY);

	EGLint major = 0, minor = 0;
	EGLBoolean res = eglInitialize(ctx->display, &major, &minor);
	assert(res);

	fprintf(stdout, "EGL v%d.%d\n", major, minor);
	fflush(stdout);

	EGLint attributes[] =
	{
		EGL_DEPTH_SIZE, 16,
		EGL_NONE,
	};
	EGLint num_config = 0;
	EGLConfig config = 0;
	res = eglChooseConfig(ctx->display, &attributes[0], &config, 1, &num_config);
	assert(res);
	assert(num_config);

	EGLint visual_id = 0;
	res = eglGetConfigAttrib(ctx->display, config, EGL_NATIVE_VISUAL_ID, &visual_id);
	assert(res);

	ctx->native_window = egl_native_window_create(ctx->native_display, 1280, 720, visual_id);
	assert(ctx->native_window);

	ctx->surface = eglCreateWindowSurface(ctx->display, config, ctx->native_window, 0);
	assert(ctx->surface != EGL_NO_SURFACE);

	ctx->context = eglCreateContext(ctx->display, config, EGL_NO_CONTEXT, 0);
	assert(ctx->context != EGL_NO_CONTEXT);

	res = eglMakeCurrent(ctx->display, ctx->surface, ctx->surface, ctx->context);
	assert(res);

	return EGL_TRUE;
}

void egl_render(struct egl_ctx *ctx)
{
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    eglSwapBuffers(ctx->display, ctx->surface);
}

int main(int argc, char *argv[])
{
	struct egl_ctx ctx = { 0 };
	EGLint res = egl_init(&ctx);
	assert(res);

	struct timespec t1, t2;

	EGLint quit = 0;
	while (!quit)
	{
		egl_render(&ctx);

		clock_gettime(CLOCK_MONOTONIC, &t1);
		long double el = 0;

		do
		{
			if (!egl_native_window_process_events(ctx.native_display, ctx.native_window))
			{
				quit = EGL_TRUE;
				break;
			}

			clock_gettime(CLOCK_MONOTONIC, &t2);
			el = t2.tv_sec - t1.tv_sec + (t2.tv_nsec - t1.tv_nsec) / NSEC;
		} while (el < 1 / FPS);
	}

	return 0;
}
