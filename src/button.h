#ifndef BUTTON_H
#define BUTTON_H
#include "texture.h"
#include "FIAL.h"

/*
 * ok, I am going to need some way for buttons to make callbacks when
 * clicked, preferably into FIAL, because, well, I just wrote the damn
 * thing!
 *
 * Also am going to need some way to load store the buttons, etc.  As
 * well as some way to save state from inside of FIAL, to allow for
 * save games, etc.
 *
 * I suppose just a generic C function that takes a void pointer could
 * work as a start....
 *
 * also, I don't know why these are floats, before I was using floats
 * as uniforms for the shaders, but now not so much.  Anyway, not a
 * big deal either way, just a tad counterintuitive.
 *
 */

struct position {
	float x, y;
};

struct dimensions {
	float w, h;
};

#define BUTTON_EVENT_MOUSEDOWN 0
#define BUTTON_EVENT_MOUSEUP   1

struct button_event {
	int type;
	struct position rel;  /*relative to the button position.*/
	struct position abs;  /*screen position ("absolute") */
};

/*
 * Initialize button_init();
 *
 * this is essetntially an interface class -- you can "inherit" from
 * it by making a factory function which creates the structure.  With
 * the appropriate values.  The "type" parameter
 *
 * An alternative to this use of OOP is to just type tag the button,
 * and then put a switch into the function that dispatches the button
 * events, and handles the destruction.  And, actually, this is
 * probably easier.  But I haven't written those quite yet, so we
 * shall see.  The main question is which is easier to deal with using
 * FIAL -- and for that I just don't know.
 *
 * I think I was thinking this would be easier, since this way if I
 * want to make a new button type, I can just do it seperately along
 * with the code that adds in the FIAL functions.  I.e., modularity.
 *
 * I think modularity is worth it in this instance, I will try and see
 * how it goes -- I feel like if I did it the type tag way, I would
 * end up with a "CUSTOM_BUTTON_USING_STRUCT_WITH_FUNCTION_POINTERS"
 * type and the only thing I will have gained is code complexity.
 */

struct button {
/*this is used for event dispatch, and display -- */

	struct position       pos;
	struct dimensions     dim;
	struct image         *img;

/*this is for event handlers -- type is just for convenience, not used
 * internally. */

	int                type;
	int (*func)(struct button_event *,
		     struct button *,
		     void *);    /*this gets called with pointer, on event.*/
	void              *ptr;            /*this gets passed to func */
	void (*destroy)(void *);           /*this gets called with ptr on destruction.*/
	int  (*copy) (void **to, void **from); /*this gets called on copy*/

/* this is for internal use */
	struct button     *next;
};

/*
 * menus are currently pretty irrelevant, might have a use for them
 * later.
 */

struct menu {
	int                      id;
	struct button      *buttons;
	struct menu           *next;
};

/*prototypes */
int fi_install_button_lib (struct FIAL_interpreter *interp,
			   struct FIAL_c_lib      **ret_lib)      ;
int button_check_overlap (struct button *b1, struct button *b2)   ;
int button_check_position(struct button *b, struct position *p2)  ;
int button_size_to_image(struct button *b)                        ;
int button_handle_event(struct button *b, struct position *pos)   ;
int menu_handle_event (struct menu *m, struct button_event *e)    ;
void destroy_button (struct button *)                             ;

#endif /*BUTTON_H*/
