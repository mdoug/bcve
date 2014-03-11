#ifndef TEXT_H
#define TEXT_H

#include <ft2build.h>
#include FT_FREETYPE_H

#include "texture.h"

/*
 *  Ok, I need a structure for procedurally created images.  Using SDL
 *  surfaces feels a bit clunky, I will just have my own.  It
 *  hopefully this is not too hard, I will start simple and concrete,
 *  since I don't know what I will eventually need.  Immediate goal is
 *  text processing from one byte per pixel luminence bitmaps.  (for
 *  alpha shading.)
 */

/* note, by and large these all take ints, however, I'm not entirely
 * sure negative numbers are always sane.  For things like border, I
 * think they more or less do what would be expected, but I haven't
 * really been testing this. */

struct string_strips {
	int longest;
	int count;
	char **strips;
};

struct type_measures {
	int width ;  /* set from typeface and string ( or string strips for max) */
	int height;  /* typeface max height, set from typeface */
	int spacer;  /* space between lines in addition to height,
			determined by user */
	int border_x;
	int border_y;

};

/* this is mostly for internal use, but here it is anyway */
struct pixel_bounding_box {
	int  x_max, x_min, y_max, y_min;
};


int init_pixel_bounding_box (struct pixel_bounding_box *pbb);
int init_freetype (void);
int allocate_image_buffer (struct image *img);
int make_test_image (struct image *img);
int write_string (struct image *img, char *str, int x_border, int y_border, int spacer);
int write_slot (struct image *img, FT_GlyphSlot slot, int offset_x, int offset_y);
int write_strip (struct image *img, struct pixel_bounding_box *pbb,
		 char *strip, int pen_x, int gpen_y);
int get_strips_from_string (struct string_strips *strips, const char *string);
int alloc_image_for_strips (struct image *img, struct string_strips *strips,
			    struct type_measures *);
int write_from_strips(struct image *img, struct string_strips *strips,
		      struct type_measures *tm);
int wrap_text_soft(char *text, int line_len);

#endif /*TEXT_H*/
