#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <inttypes.h>

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

struct gl_program_0
{
	GLuint program;
	GLint a_position;
	GLint a_color;
	GLfloat *a_positions;
	GLfloat *a_colors;
};

struct gl_ctx
{
	struct gl_program_0 *program_0;
};

EGLint egl_init(struct egl_ctx *ctx)
{
	ctx->native_display = egl_native_display_open();
	assert(eglGetError() == EGL_SUCCESS);

	ctx->display = eglGetDisplay(ctx->native_display);
	assert(eglGetError() == EGL_SUCCESS);

	EGLint major = 0, minor = 0;
	EGLBoolean res = eglInitialize(ctx->display, &major, &minor);
	assert(eglGetError() == EGL_SUCCESS);

	eglBindAPI(EGL_OPENGL_ES_API);
	assert(eglGetError() == EGL_SUCCESS);

	static const EGLint attr_config[] = {
		EGL_RED_SIZE,        1,
		EGL_GREEN_SIZE,      1,
		EGL_BLUE_SIZE,       1,
		EGL_DEPTH_SIZE,      1,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
	EGLint num_config = 0;
	EGLConfig config = 0;
	res = eglChooseConfig(ctx->display, attr_config, &config, 1, &num_config);
	assert(eglGetError() == EGL_SUCCESS);
	assert(num_config == 1);

	EGLint visual_id = 0;
	res = eglGetConfigAttrib(ctx->display, config, EGL_NATIVE_VISUAL_ID, &visual_id);
	assert(res);

	ctx->native_window = egl_native_window_create(
		ctx->native_display,
		"OpenGL ES 2.0",
		(1900 - 1280) / 2,
		(963 - 360) / 2,
		1280,
		360,
		visual_id
	);
	assert(ctx->native_window);

	ctx->surface = eglCreateWindowSurface(ctx->display, config, ctx->native_window, 0);
	assert(eglGetError() == EGL_SUCCESS);

	static const EGLint attr_context[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	ctx->context = eglCreateContext(ctx->display, config, EGL_NO_CONTEXT, attr_context);
	assert(eglGetError() == EGL_SUCCESS);

	res = eglMakeCurrent(ctx->display, ctx->surface, ctx->surface, ctx->context);
	assert(eglGetError() == EGL_SUCCESS);

	return EGL_TRUE;
}

GLuint gl_load_shader(GLenum type, const GLchar **source)
{
	GLuint shader = glCreateShader(type);
	if (!shader)
		return -1;

	glShaderSource(shader, 1, source, 0);
	glCompileShader(shader);

	GLint compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		glDeleteShader(shader);
		return -1;
	}

	return shader;
}

void gl_init(struct gl_ctx *ctx)
{
	 static const GLchar *vertex_shader_src[] =
	 {
		"attribute vec4 a_position;                           "
		"attribute vec3 a_color;                              "
		"varying vec3 v_color;                                "
		"                                                     "
		"void main() {                                        "
		"  v_color = a_color;                                 "
		"  gl_Position = a_position;                          "
		"}                                                    "
	};

	static const GLchar *fragment_shader_src[] =
	{
		"precision highp float;                               "
		"varying vec3 v_color;                                "
		"                                                     "
		"void main() {                                        "
		"  gl_FragColor = vec4(v_color, 1.0);                 "
		"}                                                    "
	};

	GLuint vertex_shader = gl_load_shader(GL_VERTEX_SHADER, (const GLchar **)vertex_shader_src);
	assert(vertex_shader != -1);

	GLuint fragment_shader = gl_load_shader(GL_FRAGMENT_SHADER, (const GLchar **)fragment_shader_src);
	assert(fragment_shader != -1);

	static struct gl_program_0 p0;
	ctx->program_0 = &p0;

	p0.program = glCreateProgram();

	glAttachShader(p0.program, vertex_shader);
	glAttachShader(p0.program, fragment_shader);

	glLinkProgram(p0.program);
	GLint linked;
	glGetProgramiv(p0.program, GL_LINK_STATUS, &linked);
	assert(linked);

	glUseProgram(p0.program);

	static GLfloat a_positions[] = {
		-1.0,  1.0,  1.0,  1.0,
		-1.0, -1.0,  1.0, -1.0,
	};

	static GLfloat a_colors[] = {
		1.0, 0.0, 0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 1.0, 1.0, 1.0, 0.0,
	};

	p0.a_positions = &a_positions[0];
	p0.a_colors = &a_colors[0];

	p0.a_position = glGetAttribLocation(p0.program, "a_position");
	glVertexAttribPointer(p0.a_position, 2, GL_FLOAT, GL_FALSE, 0, (const GLvoid *)p0.a_positions);

	p0.a_color = glGetAttribLocation(p0.program, "a_color");
	glVertexAttribPointer(p0.a_color, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid *)p0.a_colors);
}

void gl_render(struct gl_ctx *ctx)
{
    glClearColor(0, 0, 0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    {
	    glUseProgram(ctx->program_0->program);

	    GLfloat *p = &ctx->program_0->a_positions[0];
	    *p -= 0.01;
	    if (*p < -1)
	    	*p = 0;

		glEnableVertexAttribArray(ctx->program_0->a_position);
		glEnableVertexAttribArray(ctx->program_0->a_color);
	    
	    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
}

int main(int argc, char *argv[])
{
	struct egl_ctx egl_ctx = { 0 };
	EGLint res = egl_init(&egl_ctx);
	assert(res);

	printf(
		"EGL vendor: %s\n"
		"EGL version: %s\n",
		eglQueryString(egl_ctx.display, EGL_VENDOR),
		eglQueryString(egl_ctx.display, EGL_VERSION)
	);

	struct gl_ctx gl_ctx = { 0 };
	gl_init(&gl_ctx);

	struct timespec t1, t2, t4;
	uint32_t frame_count = 0;

	EGLint quit = 0;
	while (!quit)
	{
		gl_render(&gl_ctx);
		eglSwapBuffers(egl_ctx.display, egl_ctx.surface);

#ifndef _VSYNC
		do
		{
#endif
			if (!egl_native_window_process_events(egl_ctx.native_display, egl_ctx.native_window))
			{
				quit = EGL_TRUE;
				break;
			}
#ifndef _VSYNC
			const static struct timespec t3 = { .tv_sec = 0, .tv_nsec = 1000000 };
			nanosleep(&t3, 0);

			clock_gettime(CLOCK_MONOTONIC, &t2);
		} while (t2.tv_sec - t1.tv_sec + (t2.tv_nsec - t1.tv_nsec) / NSEC < 1 / FPS);
#endif

		t2 = t1;
		clock_gettime(CLOCK_MONOTONIC, &t1);

		if (frame_count++ > 0)
		{
			fprintf(
				stdout,
				"\rC: %.9Lf, A: %.9Lf                     ",
				1 / t1.tv_sec - t2.tv_sec + (t1.tv_nsec - t2.tv_nsec) / NSEC,
				frame_count / (t1.tv_sec - t4.tv_sec + (t1.tv_nsec - t4.tv_nsec) / NSEC)
			);
			fflush(stdout);
		}
		else
		{
			t4 = t1;
		}
	}

	printf("\n");

	return 0;
}
