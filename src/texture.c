#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <SDL_image.h>

#include "texture.h"

/* this is only temporary ... */

#include "text.h"

/*this is not thread safe.  To make it thread safe, add a lock!  hehe. */

int init_image(struct image *new_image, const char *filename)
{
	int n;
	static const struct bounding_box bbox = {0.0f, 1.0f, 0.0f, 1.0f};

	memset(new_image, 0, sizeof(*new_image));

	new_image->refs = 1;  /*courtesy, since the calling function gets a ref.*/
	memcpy(&new_image->bbox, &bbox, sizeof(new_image->bbox));

	if(filename)  {
		n = strlen(filename);
		new_image->filename = malloc(n + 1);
		if(!new_image->filename)
			return -1;
		strcpy(new_image->filename, filename);
	}
	return 0;
}

void destroy_image(struct image *img)
{
	if(!img)
		return;
	free(img->filename);
	SDL_free(img->surf);
	if(img->tex)
		glDeleteTextures(1, &img->tex);
	free(img);
}

void image_inc_ref (struct image *img)
{
	img->refs++;
}

void image_dec_ref (struct image *img)
{
	img->refs--;
	if(!img->refs)
		destroy_image(img);
}

/*
void delete_all_images()
{
	static struct image *iter, *tmp;
	for(iter = top_image; iter!= NULL; iter = tmp ) {
		tmp = iter->next;
		free(iter->filename);
		SDL_free(iter->surf);
		if(iter->tex)
			glDeleteTextures(1, &iter->tex);
		free(iter);
	}
	top_image = NULL;
}
*/
struct image *get_image(const char *filename)
{
	struct image *new_image = NULL;

	assert(new_image == NULL);

	new_image = calloc(1, sizeof(*new_image));
	if(!new_image)
		goto error;

	init_image(new_image, filename);
	return new_image;

error:
	if(new_image) {
		free(new_image->filename);
		SDL_free(new_image->surf);
		if(new_image->tex)
			glDeleteTextures(1, &new_image->tex);
		free(new_image);
	}
	return NULL;
}

SDL_Surface * get_surface (struct image *img)
{
	extern FILE *logfile;

	assert(img->filename);

	fprintf(logfile, "getting surface from img, filename : %s\n",
		img->filename);
	if(!img->surf)
	    /*		img->surf = SDL_LoadBMP(img->filename); */
	    img->surf = IMG_Load(img->filename);

	if(img->surf) {
		img->width = img->surf->w;
		img->height = img->surf->h;
	}

	return img->surf;
}

void delete_surface(struct image *img)
{
	if(img)
		SDL_free(img->surf);

}

static inline void set_swizzle_mask (GLint swizzlers[], SDL_PixelFormat *format)
{

	/*this is probably based on endianness -- in fact i am pretty
	 * sure that it is, but anyhow, I won't address that just yet,
	 * I will just do it for little endian machines since that's
	 * what I am running on .*/

	extern FILE *logfile;
	enum {RED, GREEN, BLUE, ALPHA};
	Uint32 img_masks[] = {format->Rmask,
			      format->Gmask,
			      format->Bmask,
			      format->Amask};

	/*this is probably due to endianness.  Comes from testing, but
	 * it makes sense with regard to the bits being flipped around...*/

	Uint32 test_masks[] = {0x000000ff,     /* red   */
			       0x0000ff00,     /* green */
			       0x00ff0000,     /* blue  */
			       0xff000000};    /* alpha */

	GLint  set_to_values[] = {GL_RED, GL_GREEN, GL_BLUE, GL_ONE};
	int i, j;
	char *labels [] = {"red", "green", "blue", "alpha"};

	memcpy(swizzlers, set_to_values, sizeof(set_to_values));

	fprintf(logfile, "logging masks for posterity...\n");
	fprintf(logfile, "BitsPerPixel = %d, BytesPerPixel = %d\n",
		format->BitsPerPixel, format->BytesPerPixel);

	for(i = 0; i < 4; i++) {
		fprintf(logfile, "%s: %x\n", labels[i], img_masks[i]);
	}
	fprintf(logfile, "and furthermore:\n");

#define LOG_BOOL(x) do{fprintf(logfile, #x " is %s\n", x ? "true" : "false");}while(0)

	LOG_BOOL(SDL_PIXELTYPE(format->format) == SDL_PIXELTYPE_UNKNOWN);

	LOG_BOOL(SDL_PIXELTYPE(format->format) == SDL_PIXELTYPE_PACKED8);
	LOG_BOOL(SDL_PIXELTYPE(format->format) == SDL_PIXELTYPE_PACKED16);
	LOG_BOOL(SDL_PIXELTYPE(format->format) == SDL_PIXELTYPE_PACKED32);

	LOG_BOOL(SDL_PIXELTYPE(format->format) == SDL_PIXELTYPE_ARRAYU8);
	LOG_BOOL(SDL_PIXELTYPE(format->format) == SDL_PIXELTYPE_ARRAYU16);
	LOG_BOOL(SDL_PIXELTYPE(format->format) == SDL_PIXELTYPE_ARRAYU32);
	LOG_BOOL(SDL_PIXELTYPE(format->format) == SDL_PIXELTYPE_ARRAYF16);
	LOG_BOOL(SDL_PIXELTYPE(format->format) == SDL_PIXELTYPE_ARRAYF32);


	fprintf(logfile, "thank you for your cooperation!\n");

	for(i = 0; i < 4; i++) {
		for(j = 0; j < 4; j++) {
			if(img_masks[i] == test_masks[j]) {
				swizzlers[i] = set_to_values[j];
				fprintf(logfile, "set %s to %s\n", labels[i], labels[j]);
				break;
			}
		}
	}

	return;
}

GLuint get_texture (struct image *img)
{
	extern FILE *logfile;
	GLenum format;

	/* these swizzle masks where just arrived at via trial and
	 * error.  the right way would be to deduce it via color masks
	 * at runtime, then it would work for any image file supported
	 * by SDL_image.
	 *
	 * i'll do that tomorrow.  I will also start thinking about
	 * how to get the game set up, and the menus and stuff.  I
	 * haven't actually done this part before, like ever, with any
	 * language -- though, honestly, I feel like fail is pretty
	 * good.  Still some warts, but generally pretty good.
	 *
	 * and, of course, I need to fix errors in there.
	 */

	GLint swizzle_mask[4];
	void *pixels;

	if(img->tex)
		return img->tex;
	if(!img->type && !img->surf)
		if(!get_surface(img))
			return 0;
	fprintf(logfile, "loading image %s\n", img->filename);
	assert((!img->type && img->surf) || (img->type && img->buf));


	glGenTextures(1, &img->tex);
	if(!img->tex) {
		return 0;
	}
	glBindTexture(GL_TEXTURE_2D, img->tex);
	if(img->type) {
		GLint temp_swizzle[] = {GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA};
		format = GL_RED;
		memcpy(swizzle_mask, temp_swizzle, sizeof(swizzle_mask));
		pixels = img->buf;
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	} else 	if(img->surf->format->BytesPerPixel == 3) {
		format = GL_RGB;
		set_swizzle_mask(swizzle_mask, img->surf->format);
		pixels = img->surf->pixels;
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	} else {
		assert(0);
		assert(img->surf->format->BytesPerPixel == 4);
		format = GL_RGBA;
		set_swizzle_mask(swizzle_mask, img->surf->format);
		pixels = img->surf->pixels;
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	}
	glTexImage2D(GL_TEXTURE_2D, 0, format, img->width, img->height,  0,
		     format, GL_UNSIGNED_BYTE, pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle_mask);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,  GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,  GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, 0);

	/*delete surface here? probably better to leave that to be
	 * called explicitly..*/
	return img->tex;
}

void delete_texture(struct image *img)
{
	if(!img->tex)
		return;
	glDeleteTextures(1, &img->tex);
	return;
}
