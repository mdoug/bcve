#ifndef TEXTURE_H
#define TEXTURE_H

#include <SDL.h>
#include <GL/glew.h>

/*
 *  For now I am just going to do basic reference counting for these.
 *  This is obviously not a robust solution, but throwing locks on it
 *  isn't really necessary, since the main thread will be handling all
 *  of these things.  But that is the next step if I decide to have
 *  multiple threads.
 */

#define IMAGE_TYPE_SDL_SURFACE 0
#define IMAGE_TYPE_BUF         1

struct bounding_box {
	float           x_min;
	float           x_max;
	float           y_min;
	float           y_max;
};

struct image {
	int type;

	int           refs;
	struct image *next;

	char *filename;
	struct bounding_box bbox;

	GLuint tex;
	int width, height;

	SDL_Surface *surf;
	char        *buf;
};

struct image *get_image(const char *filename);
int init_image(struct image *new_image, const char *filename);
void destroy_image(struct image *img);
/*void delete_all_images();*/
SDL_Surface * get_surface (struct image *img);
void delete_surface(struct image *img);
GLuint get_texture (struct image *img);
void delete_texture(struct image *img);
void image_inc_ref (struct image *img);
void image_dec_ref (struct image *img);

#endif /*TEXTURE_H*/
