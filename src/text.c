#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <ctype.h>

#include "texture.h"
#include "text.h"

static FT_Library freetype_library;
static FT_Face    default_typeface;

int init_freetype (void)
{
	int ret;

	ret = FT_Init_FreeType(&freetype_library);
	if(ret) {
		return -1;
	}
	ret = FT_New_Face (freetype_library,
			   "DejaVuSansMono.ttf",
			   0,
			   &default_typeface);
	if(ret) {
		/* FIXME: switch on error type */
		return -1;
	}

	/*defaults to 72 dpi..., no idea what I'm doing so I'll try
	  that first for last two arguments  */
	FT_Set_Char_Size(default_typeface, 24 * 64, 0, 0, 0);
	return 0;
}

/* this allocates an image to store text to, and initializes to 0. */

int init_pixel_bounding_box (struct pixel_bounding_box *pbb)
{
	pbb->x_min = pbb->y_min = INT_MAX;
	pbb->x_max = pbb->y_max = INT_MIN;
	return 0;
}

int allocate_image_buffer (struct image *img)
{
	assert(img->width && img->height);
	assert(!img->buf);
	assert(!img->surf);

	img->buf = calloc(1, img->width * img->height);
	if(!img->buf)
		return -1;
	return 0;
}

/* FIXME: errors. */

int write_string (struct image *img, char *str, int x_border, int y_border, int spacer)
{
	FT_Face font = default_typeface;
	struct string_strips strips;
	struct type_measures tm;

	tm.width = font->max_advance_width;
	tm.height = font->height;
	tm.spacer = spacer;
	tm.border_x = x_border;
	tm.border_y = y_border;

	get_strips_from_string(&strips, str);
	alloc_image_for_strips(img, &strips, &tm);
	write_from_strips(img, &strips, &tm);

	return 0;
}

int write_from_strips(struct image *img, struct string_strips *strips,
		      struct type_measures *tm)
{
	char **iter, **end;
	int pen_x, pen_y;
	struct pixel_bounding_box pbb;
	float tmp;

	init_pixel_bounding_box(& pbb);

	pen_x = tm->border_x;
	pen_y = tm->border_y;

	for(iter = strips->strips, end = strips->strips + strips->count;
	    iter != end; ++iter) {
		write_strip(img, &pbb, *iter, pen_x, pen_y);
		pen_y += (tm->height >> 6) + tm->spacer;
	}

	//	pen_y += (tm->height >> 6) + tm->spacer;
	pbb.y_max = pen_y > pbb.y_max ? pen_y : pbb.y_max;

	pbb.x_min -= tm->border_x;
	pbb.x_max += tm->border_x;
	pbb.y_min -= tm->border_y;
	pbb.y_max += tm->border_y;

	assert(img->width && img->height);
	if(!img->width || !img->height)
		return 1; /* is this an error?  maybe, don't know */

	tmp = (float)(img->width);
	img->bbox.x_min = ((float) pbb.x_min) / tmp;
	img->bbox.x_max = ((float) pbb.x_max) / tmp;

	tmp = (float)(img->height);
	img->bbox.y_min = ((float) pbb.y_min) / tmp;
	img->bbox.y_max = ((float) pbb.y_max) / tmp;

	return 0;
}

int get_strips_from_string (struct string_strips *strips, char const *string)
{
	int ret = -1; /*return */
	int i = 0 , len = 0, tmp_len = 0, count = 0;
	const char *c = NULL;

	assert(strips && string);

	memset(strips, 0, sizeof(*strips));

	count = 0;
	for(c = string; *c != '\0'; c++) {
		if(*c == '\n') {
			count++;
			len = tmp_len > len ? tmp_len : len;
			tmp_len = 0;
		} else {
			tmp_len++;
		}
	}
	count++;
	strips->strips = calloc(count, sizeof(char *));
	if(!strips->strips)
		return -1;
	strips->count = count;
	strips->longest = len;

	c = string;
	for(i = 0; i < count; i++) {
		int j = count - 1 - i;
		char *to;
		/* simplest, not gonna over think this */
		strips->strips[j] = malloc(len + 1);
		if(!strips->strips[j]) {
			ret = -1;
			goto cleanup;
		}
		for(to = strips->strips[j]; (*c != '\n' && *c != '\0'); c++, to++)
			*to = *c;
		*to = '\0';
		if(*c)
			c++;
	}
	return 0;
cleanup:
	if(strips->strips) {
		int i;
		for(i = 0; i < strips->count; i++)
			free(strips->strips[i]);
	}
	free (strips->strips);
	memset(strips, 0, sizeof(*strips));
	return ret;
}

int alloc_image_for_strips (struct image *img, struct string_strips *strips,
			    struct type_measures *tm)
{
	assert(img);
	assert(strips);

	img->width = (tm->width >> 6) *  strips->longest + 2 * tm->border_x;

	if(tm->spacer >= 0 || tm->border_y > tm->height) {
		img->height = ((tm->height >> 6) + tm->spacer) * strips->count +
			2 *tm->border_y;
	} else {
		img->height = ((tm->height >> 6) + tm->spacer) * strips->count +
			2 * tm->border_y;
	}

	return allocate_image_buffer(img);
}

int write_strip (struct image *img, struct pixel_bounding_box *pbb,
		 char *strip, int pen_x, int pen_y)
{
	int ret;
	FT_GlyphSlot slot = default_typeface->glyph;

	pbb->x_min = pen_x < pbb->x_min ? pen_x : pbb->x_min;
	pbb->y_min = pen_y < pbb->y_min ? pen_y : pbb->y_min;

	assert(img);
	assert(img->buf);
	assert(strip);

	for(; *strip != '\0'; ++strip) {
		if((ret = FT_Load_Char(default_typeface, *strip, FT_LOAD_RENDER)))
			exit(3);
		write_slot(img, slot, pen_x, pen_y);
		pen_x += slot->advance.x >> 6;
		pen_y += slot->advance.y >> 6;
	}

	pbb->x_max = pen_x > pbb->x_max ? pen_x : pbb->x_max;
	pbb->y_max = pen_y > pbb->y_max ? pen_y : pbb->y_max;

	return 0;
}

int write_slot (struct image *img, FT_GlyphSlot slot, int offset_x, int offset_y)
{
	int i;

	int start_offset = offset_x + (offset_y * img->width);

	for( i = 0; i < slot->bitmap.rows; i++ ) {
		int bearing = slot->metrics.horiBearingY;
		int height  = slot->metrics.height;
		int height_adjust = ((bearing - height) >> 6) * img->width ;

		memcpy(img->buf + start_offset + (img->width * i) + height_adjust  ,
		       slot->bitmap.buffer + (slot->bitmap.width * (slot->bitmap.rows - 1 - i)),
		       slot->bitmap.width);
	}

	return 0;
}


int make_test_image (struct image *img)
{

	assert(img);
	init_image(img, NULL);
	img->type = IMAGE_TYPE_BUF;
	char str[] = "hold left mouse button to pan, "
		"right button to zoom, "
		"any key to quit.  I think I will add in more text stuff soon.  I like text.  And buttons.";

	wrap_text_soft(str, 40);
	write_string (img,str, 20, 10, -15);


/*#define NO_STRIP_STUFF
  #define CHECKER_BOARD */

#ifdef NO_STRIP_STUFF
	h = img->height = 400;
	w = img->width  = 400;

	if((ret = allocate_image_buffer(img)))
		return ret;


#endif /* NO_STRIP_STUFF */

#ifdef CHECKER_BOARD

	for(i = 0; i < h * w; i++) {
		int x = i % w;
		int y = i / h;

		int x_odd = (x / 40) % 2;
		int y_odd = (y / 40) % 2;

		if((x_odd && y_odd) || (!x_odd && !y_odd)) {
			memset(img->buf + i, 0xff, 1);
		} else {
			memset(img->buf + i, 0x00, 0);
		}
	}

#endif /*CHECKER_BOARD*/

	return 0;
}

/*
 * this will not break words that are longer than the wrap length --
 * hence the name "soft".  It is a good algorithm because it wraps the
 * string, in place, without changing its length.  Not exactly
 * perfect, but pretty good, and very efficient.
 */

int wrap_text_soft(char *text, int line_len)
{
	char *c, *last_space;
	int col;

	for(c = text, col = 1, last_space = NULL; *c != '\0'; c++, col++) {
		if(!isspace(*c))
			continue;
		if(*c == '\n') {
			col = 0;
			continue;
		}
		assert(isspace(*c) && *c != '\n');
		if(col < line_len) {
			last_space = c;
			continue;
		}
		if(last_space){
			col = c - last_space;
			*last_space = '\n';
			last_space = NULL;
			continue;
		}
		assert(last_space == NULL);
		/* nothing to be done... */
		continue;
	}
	return 0;
}
