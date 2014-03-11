#ifndef BCVE_H
#define BCVE_H
#include <stdio.h>

#include "button.h"
#include "texture.h"
#include "shader.h"

/*
 *  Structure Definitions
 */

struct window_info {
	SDL_Window *            window;
	SDL_GLContext          context;
	int              width, height;
	FILE*                  logfile;
};

struct button_draw_stuff {
	GLuint                 vertex_buf;
	GLuint                  index_buf;
	GLuint               vertex_array;
	struct gfx_program *      program;

	GLint               screen_w_loc;
	GLint               screen_h_loc;
	GLint               button_w_loc;
	GLint               button_h_loc;
	GLint                image_w_loc;
	GLint                image_h_loc;
	GLint                    pos_loc;
	GLint                 offset_loc;
	GLint                  scale_loc;
};

void start_viewer (struct window_info *win_info);

struct button *create_psb_button(struct position    *pos,
				 struct dimensions  *dim,
				 const char *filename);
int init_button_gfx_program (struct button_draw_stuff *bds,
				FILE *logfile);
int append_to_exec_path (char *buf, const char *str, int max_size);

#endif /*BCVE_H*/
