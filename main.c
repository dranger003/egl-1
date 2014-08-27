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
	assert(eglGetError() == EGL_SUCCESS);

	ctx->display = eglGetDisplay(ctx->native_display);
	assert(eglGetError() == EGL_SUCCESS);

	EGLint major = 0, minor = 0;
	EGLBoolean res = eglInitialize(ctx->display, &major, &minor);
	assert(eglGetError() == EGL_SUCCESS);

	fprintf(stdout, "EGL v%d.%d\n", major, minor);
	fflush(stdout);

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

void gl_init()
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

	GLuint program = glCreateProgram();

	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	glLinkProgram(program);
	GLint linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	assert(linked);

	glUseProgram(program);

	static const GLfloat a_positions[] = {
		-1.0,  1.0,  1.0,  1.0,
		-1.0, -1.0,  1.0, -1.0,
	};

	static const GLfloat a_colors[] = {
		1.0, 0.0, 0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 1.0, 1.0, 1.0, 0.0,
	};

	GLint a_position = glGetAttribLocation(program, "a_position");
	glVertexAttribPointer(a_position, 2, GL_FLOAT, GL_FALSE, 0, (const GLvoid *)&a_positions[0]);
	glEnableVertexAttribArray(a_position);

	GLint a_color = glGetAttribLocation(program, "a_color");
	glVertexAttribPointer(a_color, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid *)&a_colors[0]);
	glEnableVertexAttribArray(a_color);

}

void gl_render()
{
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

int main(int argc, char *argv[])
{
	struct egl_ctx ctx = { 0 };
	EGLint res = egl_init(&ctx);
	assert(res);

	gl_init();

	struct timespec t1, t2;

	EGLint quit = 0;
	while (!quit)
	{
		gl_render();

		eglSwapBuffers(ctx.display, ctx.surface);

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
