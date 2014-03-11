#ifndef SHADER_H
#define SHADER_H

/*
 *  this is for shader handling....
 */

#define TEXT_BUF_START_SIZE 1024

/*text buffers will be initialized to 0, since otherwise after every
  character the trailing /0 will have to be written.   */

 struct text_buf {
	 char *buf;
	 size_t size, cap; };

/*
 * I could just use the GL ones, but what the hell.
 */


/*
 *  if error is null, then there was no compilation error.  otherwise,
 *  there was.
 */

#define SHADER_VERTEX     0
#define SHADER_FRAGMENT   1

struct shader {
	int type;
	GLuint id;
	char *error;
};

struct shader_list {
	GLuint id;
	struct shader_list *next;
};

struct gfx_program {
	GLuint id;
	struct shader_list *shaders;
	char *error;
};

void add_char (struct text_buf *buffy, char c);
void load_file_to_buf(struct text_buf *buf, FILE *f);
struct shader *create_shader(FILE *f, int type);
void destroy_shader(struct shader *);
struct gfx_program *create_gfx_program( void );
int gfx_add_shader (struct gfx_program *, struct shader *);
int gfx_link (struct gfx_program *p);

#endif /*SHADER_H*/
