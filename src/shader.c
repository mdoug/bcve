#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <GL/glew.h>

#include "shader.h"

FILE* logfile;

void add_char (struct text_buf *buffy, char c)
{
	assert(buffy);
	if(!buffy->buf) {
		assert(!buffy->size);
		assert(!buffy->cap);
		buffy->buf = malloc(TEXT_BUF_START_SIZE);
		//FIXME: error checks, yadayada....
		memset(buffy->buf, 0, TEXT_BUF_START_SIZE);
		buffy->cap = TEXT_BUF_START_SIZE;
	}
	assert(buffy->buf);
	assert(buffy->cap);
	assert(buffy->cap > buffy->size);

	if(buffy->size == buffy->cap - 1) {
		char *tmp = realloc(buffy->buf, buffy->cap * 2);
		assert(tmp);
		buffy->buf = tmp;
		memset(buffy->buf + buffy->cap, 0, buffy->cap);
		buffy->cap *= 2;
	}
	buffy->buf[buffy->size++] = c;
	return;
}

void load_file_to_buf(struct text_buf *buf, FILE *f)
{
	assert(buf);
	assert(f);

	int c;
	while((c = fgetc(f)) != EOF)
		add_char(buf, c);

}

struct shader *create_shader(FILE *f, int type)
{
	struct shader *s = malloc(sizeof(*s));
	struct text_buf buffy;
	GLenum shader_type;
	GLint tmp = 0;

	memset(&buffy, 0, sizeof(buffy));

	if(!s)
		goto error;
	memset(s, 0, sizeof(*s));

	switch(type) {
	case SHADER_VERTEX:
		shader_type = GL_VERTEX_SHADER;
		break;
	case SHADER_FRAGMENT:
		shader_type = GL_FRAGMENT_SHADER;
		break;
	default:
		assert(0);
		goto error;
		break;
	}

	load_file_to_buf(&buffy, f);
	fprintf(logfile, "%s", buffy.buf);
	s->id = glCreateShader(shader_type);
	glShaderSource(s->id, 1, (const GLchar **)&buffy.buf, NULL);
	glCompileShader(s->id);
	glGetShaderiv(s->id, GL_COMPILE_STATUS, &tmp);

	if(!tmp) {
		tmp = 0;
		glGetShaderiv(s->id, GL_INFO_LOG_LENGTH, &tmp);
		s->error = malloc(tmp);
		if(!s->error) {
			/*this is not my day.... bugging out*/
			goto error;
		}
		glGetShaderInfoLog(s->id, tmp, NULL, s->error);
	}

	free(buffy.buf);
	return s;

error:
	free(buffy.buf);
	if(s) {
		if(!s->id) {
			glDeleteShader(s->id);
		}
		free(s->error);
		free(s);
	}
	return NULL;
}

void destroy_shader(struct shader *s)
{
	if(s) {
		free(s->error);
		glDeleteShader(s->id);
		free(s);
	}
}

struct gfx_program *create_gfx_program( void )
{
	struct gfx_program *p = malloc(sizeof(*p));
	if(!p)
		return NULL;
	memset(p, 0, sizeof(*p));
	p->id = glCreateProgram();
	if(!p->id) {
		free(p);
		return NULL;
	}
	return p;
}

int gfx_add_shader (struct gfx_program *p, struct shader *s)
{
	struct shader_list *tmp;
	if(!p || !s)
		return -1;
	tmp = malloc(sizeof(*tmp));
	if(!tmp)
		return -1;
	memset(tmp, 0, sizeof(*tmp));
	tmp->id = s->id;
	tmp->next = p->shaders;
	p->shaders = tmp;

	glAttachShader(p->id, s->id);

	return 0;
}

int gfx_link (struct gfx_program *p)
{
	GLint tmp = 0;
	struct shader_list *iter = NULL, *iter2 = NULL;

	assert(p);

	glLinkProgram (p->id);

	glGetProgramiv(p->id, GL_LINK_STATUS, &tmp);
	if(!tmp) {
		glGetProgramiv(p->id, GL_INFO_LOG_LENGTH, &tmp);
		p->error = malloc(tmp);
		if(!p->error)
			return -1;
		glGetProgramInfoLog(p->id, tmp, NULL, p->error);
		return -1;
	}

	iter = p->shaders;
	while(iter) {
		iter2 = iter->next;
		glDetachShader(p->id, iter->id);
		free(iter);
		iter = iter2;
	}
	p->shaders = NULL;

	return 0;
}

int destroy_gfx_program (struct gfx_program *p)
{
	if(p) {
		struct shader_list *iter, *tmp;
		for(iter = p->shaders; iter != NULL; iter = tmp) {
			tmp = iter->next;
			free(iter);
		}
		glDeleteProgram (p->id);
		free(p->error);
		free(p);
	}
	return 0;
}
