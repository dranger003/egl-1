#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <inttypes.h>
#include <string.h>

#include <X11/Xutil.h>
#include "eglnative.h"

#include <GLES2/gl2.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#define NSEC		1000000000.0L
#define FPS			60.0L

#define FONT_FILE	"/usr/share/fonts/TTF/LiberationMono-Regular.ttf"
#define FONT_SIZE	96

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

struct gl_program_1
{
	GLuint program;
};

struct gl_program_2
{
	GLuint program;
	GLint a_position;
	GLint u_texture;
	GLint u_color;
};

struct ft_ctx
{
	FT_Library library;
	FT_Face face;
	const char * const font_file;
};

struct gl_ctx
{
	uint64_t frame;
	struct gl_program_0 *program_0;
	struct gl_program_1 *program_1;
	struct gl_program_2 *program_2;
	struct ft_ctx *ft_ctx;
};

static int width = 1280;
static int height = 360;

void egl_native_window_resize(EGLint new_width, EGLint new_height)
{
	width = new_width;
	height = new_height;

    glViewport(0, 0, width, height);
}

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
		(1900 - width) / 2,
		(963 - height) / 2,
		width,
		height,
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

GLuint gl_compile_shader(GLenum type, const GLchar * const *source)
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

GLuint gl_create_program(const GLchar * const *vertex_shader_source, const GLchar * const *fragment_shader_source)
{
	GLuint vertex_shader = gl_compile_shader(GL_VERTEX_SHADER, vertex_shader_source);
	assert(vertex_shader != -1);

	GLuint fragment_shader = gl_compile_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
	assert(fragment_shader != -1);

	GLuint program = glCreateProgram();

	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	glLinkProgram(program);
	GLint linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	assert(linked);

	return program;
}

void gl_init(struct gl_ctx *ctx)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	{
		static struct gl_program_0 p0;
		ctx->program_0 = &p0;

		static const GLchar * const vertex_shader_source[] =
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

		static const GLchar * const fragment_shader_source[] =
		{
			"precision highp float;                               "
			"varying vec3 v_color;                                "
			"                                                     "
			"void main() {                                        "
			"  gl_FragColor = vec4(v_color, 1.0);                 "
			"}                                                    "
		};

		p0.program = gl_create_program(vertex_shader_source, fragment_shader_source);

		static GLfloat a_positions[] = {
			-1.0,  1.0,  1.0,  1.0,
			-1.0, -1.0,  1.0, -1.0,
		};

		static GLfloat a_colors[] = {
			1.0, 0.0, 0.0, 0.0, 1.0, 0.0,
			0.0, 0.0, 1.0, 1.0, 1.0, 0.0,
		};

		p0.a_position = glGetAttribLocation(p0.program, "a_position");
		p0.a_color = glGetAttribLocation(p0.program, "a_color");

		p0.a_positions = &a_positions[0];
		p0.a_colors = &a_colors[0];
	}

	{
		static struct gl_program_1 p1;
		ctx->program_1 = &p1;

		static const GLchar * const vertex_shader_source[] =
		{
			"attribute vec4 a_position;  "
			"                            "
			"void main() {               "
			"  gl_Position = a_position; "
			"}                           "
		};

		static const GLchar * const fragment_shader_source[] =
		{
			"precision highp float;               "
			"                                     "
			"void main() {                        "
			"  gl_FragColor = vec4(0, 0, 0, 0.4); "
			"}                                    "
		};

		p1.program = gl_create_program(vertex_shader_source, fragment_shader_source);
	}

	{
		static struct gl_program_2 p2;
		ctx->program_2 = &p2;

		static const GLchar * const vertex_shader_source[] =
		{
			"attribute vec4 a_position;                 "
			"varying vec2 v_position;                   "
			"                                           "
			"void main() {                              "
			"  gl_Position = vec4(a_position.xy, 0, 1); "
			"  v_position = a_position.zw;              "
			"}                                          "
		};

		static const GLchar * const fragment_shader_source[] =
		{
			"precision highp float;                                                        "
			"varying vec2 v_position;                                                      "
			"uniform sampler2D u_texture;                                                  "
			"uniform vec4 u_color;                                                         "
			"                                                                              "
			"void main() {                                                                 "
			"  gl_FragColor = vec4(1, 1, 1, texture2D(u_texture, v_position).a) * u_color; "
			"}                                                                             "
		};

		p2.program = gl_create_program(vertex_shader_source, fragment_shader_source);

		p2.a_position = glGetAttribLocation(p2.program, "a_position");
		p2.u_texture = glGetUniformLocation(p2.program, "u_texture");
		p2.u_color = glGetUniformLocation(p2.program, "u_color");
	}
}

void gl_render(struct gl_ctx *ctx)
{
    glClearColor(1, 1, 1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(ctx->program_0->program);
    {
		glVertexAttribPointer(ctx->program_0->a_position, 2, GL_FLOAT, GL_FALSE, 0, (const GLvoid *)ctx->program_0->a_positions);
		glEnableVertexAttribArray(ctx->program_0->a_position);

		glVertexAttribPointer(ctx->program_0->a_color, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid *)ctx->program_0->a_colors);
		glEnableVertexAttribArray(ctx->program_0->a_color);
	    
	    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

	glUseProgram(ctx->program_1->program);
	{
		static const GLfloat a_positions[] =
		{
			-0.5,  0.5,  0.5,  0.5,
			-0.5, -0.5,  0.5, -0.5,
		};

		GLint a_position = glGetAttribLocation(ctx->program_1->program, "a_position");
		glVertexAttribPointer(a_position, 2, GL_FLOAT, GL_FALSE, 0, (const GLvoid *)&a_positions[0]);
		glEnableVertexAttribArray(a_position);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	glUseProgram(ctx->program_2->program);
	{
		static const GLfloat u_color[] = { 1, 1, 1, 1 };
		glUniform4fv(ctx->program_2->u_color, 1, u_color);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ctx->program_2->u_texture);
		glUniform1i(ctx->program_2->u_texture, 0);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		static int c = 0;
		static char t[128];
		if (!(ctx->frame++ % 2))
			++c;
		sprintf(t, "%d", c);

		FT_GlyphSlot g = ctx->ft_ctx->face->glyph;
		GLfloat sx = 2.0 / width, sy = 2.0 / height;
		GLfloat x = -0.5 + (10 * sx), y = -0.5 + (10 * sy) + ((FONT_SIZE * 0.75) * sy);

		glEnableVertexAttribArray(ctx->program_2->a_position);

		for (const char *p = &t[0]; *p; p++)
		{
			if (FT_Load_Char(ctx->ft_ctx->face, *p, FT_LOAD_RENDER))
			{
				assert(0);
				continue;
			}

			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_ALPHA,
				g->bitmap.width,
				g->bitmap.rows,
				0,
				GL_ALPHA,
				GL_UNSIGNED_BYTE,
				g->bitmap.buffer
			);

			GLfloat x2 = x + g->bitmap_left * sx;
			GLfloat y2 = y - g->bitmap_top * sy;
			GLfloat w = g->bitmap.width * sx;
			GLfloat h = g->bitmap.rows * sy;

			GLfloat a_positions[][4] =
			{
				{     x2,     -y2, 0, 0 },
				{ x2 + w,     -y2, 1, 0 },
				{     x2, -y2 - h, 0, 1 },
				{ x2 + w, -y2 - h, 1, 1 },
			};

			glVertexAttribPointer(ctx->program_2->a_position, 4, GL_FLOAT, GL_FALSE, 0, (const GLvoid *)&a_positions[0][0]);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			x += (g->advance.x >> 6) * sx;
			y += (g->advance.y >> 6) * sy;
		}
	}
}

void ft_init(struct ft_ctx *ctx)
{
	FT_Error res = FT_Init_FreeType(&ctx->library);
	assert(res == 0);

	res = FT_New_Face(ctx->library, ctx->font_file, 0, &ctx->face);
	assert(res == 0);
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

	static struct ft_ctx ft_ctx = { .font_file = FONT_FILE };
	ft_init(&ft_ctx);

	FT_Set_Pixel_Sizes(ft_ctx.face, 0, FONT_SIZE);
	gl_ctx.ft_ctx = &ft_ctx;

	struct timespec t1, t2, t3;
	uint32_t frame_count = 0;

	EGLBoolean quit = EGL_FALSE;
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
			const static struct timespec t4 = { .tv_sec = 0, .tv_nsec = 1000000 };
			nanosleep(&t4, 0);

			clock_gettime(CLOCK_MONOTONIC, &t2);
			t2.tv_nsec += t4.tv_nsec * 0.7;
		} while (t2.tv_sec - t1.tv_sec + (t2.tv_nsec - t1.tv_nsec) / NSEC < 1.0L / FPS);
#endif

		t2 = t1;
		clock_gettime(CLOCK_MONOTONIC, &t1);

		if (frame_count++ > 0)
		{
			fprintf(
				stdout,
				"\rC: %.9Lf, A: %.9Lf                     ",
				1 / (t1.tv_sec - t2.tv_sec + (t1.tv_nsec - t2.tv_nsec) / NSEC),
				frame_count / (t1.tv_sec - t3.tv_sec + (t1.tv_nsec - t3.tv_nsec) / NSEC)
			);
			fflush(stdout);
		}
		else
		{
			t3 = t1;
		}
	}

	printf("\n");

	return 0;
}
