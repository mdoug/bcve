#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "button.h"

/*
 * this is awful.  I just want to write some FIAL interface
 * functions.....
 */

#include "FIAL.h"
#include "ast.h"
#include "interp.h"
#include "error_def_short.h"
#include "error_macros.h"

int perform_fi_button_callback (struct button_event *be,
				struct button        *b,
				void               *ptr);


struct button *copy_button (struct button *from)
{
	struct button *to = malloc(sizeof(*to));
	if(!to)
		return NULL;
	memcpy(to, from, sizeof(*to));
	to->next = NULL;

	if(to->img) {
		image_inc_ref(to->img);
	}

	if(from->ptr && from->copy) {
		from->copy(&(to->ptr), &(from->ptr));
	}
	return to;
}

void destroy_button (struct button *but)
{
	if(!but)
		return;

	if(but->img) {
		image_dec_ref(but->img);
	}

	if(but->destroy)
		but->destroy(but->ptr);

	/*image decrease ref or something of the sort should go
	 * here..*/

	free(but);
	return;
}

int button_check_overlap (struct button *b1, struct button *b2)
{
	if(b1->pos.x + b1->dim.w < b2->pos.x ||
	   b2->pos.x + b2->dim.w < b1->pos.x ||
	   b1->pos.y + b1->dim.w < b2->pos.y ||
	   b2->pos.y + b2->dim.w < b1->pos.y)
		return 0;

	return 1;
}

int button_check_position(struct button *b, struct position *p)
{
	if(p->x >= b->pos.x && p->x <= b->pos.x + b->dim.w &&
	   p->y >= b->pos.y && p->y <= b->pos.y + b->dim.h)
		return 1;
	return 0;
}

int button_size_to_image(struct button *b)
{
	int h, w;
	float h_f, w_f;
	assert(b);

	if(!b->img)
		return -1;
	assert(b->img);
	w_f = (float) abs(((float) b->img->width) * (b->img->bbox.x_max -
						     b->img->bbox.x_min));
	b->dim.w = (int)floor(w_f + .5); /* round it off... */

	h_f = (float) abs(((float) b->img->height) * (b->img->bbox.y_max -
						      b->img->bbox.y_min));
	b->dim.h = (int)floor(h_f + .5); /* round it off... */

	return 0;
}

/*call this on match */

/*return 1 to capture the event. */

int button_handle_event(struct button *b, struct position *ignored)
{
	extern FILE *logfile;

	if(!b)
		return 0;
	if(!b->func)
		return 0;
	fprintf(logfile, "asked to handle event.  b: %p, b->func = %p, "
		"b->ptr = %p, perform_fi_button_callback = %p", b, b->func,
		b->ptr, perform_fi_button_callback);
	return b->func(NULL, b, b->ptr);


}

/* menu layers are not currently implemented, so return values don't
 * matter, because buttons do not overlap.  */
/*
int menu_handle_event (struct menu *m, struct button_event *e)
{
	struct button *iter;
	for(iter = m->buttons; iter != NULL; iter = iter->next) {
		if(button_check_position(iter, &e->abs))
			return button_handle_event(&e->abs, e);
	}
	return 0;
}
*/

/***********************************************************************
 *								       *
 *   Fail stuff. 						       *
 *								       *
 *    also adding a global variable that represents the current        *
 *    buttons.  This is likely not a permanent solution.  	       *
 *    								       *
 ***********************************************************************/

/***********************************************************************
 *								       *
 *  this code is using its own logfile for error reporting instead of  *
 *  using the FIAL error mechanisms, because those lacked existence    *
 *  when it was written.  This should get fixed at some point.	       *
 *								       *
 ***********************************************************************/

struct button *buttons = NULL;

static int button_type = 0;

int fi_clear_buttons (int argc, struct FIAL_value **args,
		     struct FIAL_exec_env *env,  void *ptr)
{
	extern FILE *logfile;
	fprintf(logfile, "in clear buttons....\n");
	while(buttons) {
		struct button *tmp = buttons->next;
		destroy_button(buttons);
		buttons = tmp;
	}
	fprintf(logfile, "cleared buttons.  buttons : %p", buttons);
	return 0;
}

int fi_create_button(int argc, struct FIAL_value **args,
		     struct FIAL_exec_env *env,  void *ptr)
{
	extern FILE *logfile;
	struct FIAL_value *ret = NULL;
	struct button     *but = NULL;
	char  *filename = NULL;

	fprintf(logfile, "creating button\n");

	if(argc == 0 || !args) {
		fprintf(logfile, "error, no args in create button!\n");
		return 1;
	}
	assert(args);

	ret = args[0];

	FIAL_clear_value(ret, env->interp);

	assert(button_type);
	ret->type = button_type;
	ret->ptr  = malloc(sizeof(struct button));
	if(!ret->ptr) {
		fprintf(logfile, "error, couldn't allocate button in arg[0] in "
			"create button!\n");
		memset(ret, 0, sizeof(*ret));
		return 1;
	}
	memset(ret->ptr, 0, sizeof(struct button));
	but = ret->ptr;

	/* this is needlessly complicated, sorry, relies on logical
	 * disjunctive evaluation order to set filename.  */

	if(argc > 1
	   &&
	   ((args[1]->type == VALUE_STRING && (filename = args[1]->str)) ||
	    (args[1]->type == VALUE_TEXT_BUF&&(filename = args[1]->text->buf)))){
		but->img = get_image(filename);
		get_surface (but->img);
		get_texture (but->img);

		but->dim.w = but->img->width;
		but->dim.h = but->img->height;

		fprintf(logfile, "got image, h: %d,  w: %d\n",
			but->img->height, but->img->width);
	} else {
		fprintf(logfile, "did not load an image :(\n");

	}

	fprintf(logfile,"created button, set ret->type to %d.\n", ret->type);
	fprintf(logfile, "  but:\ndim.w : %f, img : %p "
		"dim.h : %f, pos.x : %f, pos.y %f\n", but->dim.w, but->img,
		but->dim.h, but->pos.x,	but->pos.y);
	return 0;
}

int fi_copy_button (int argc, struct FIAL_value **argv,
		    struct FIAL_exec_env *env, void *ptr)
{
	if(argc < 2 || argv[1]->type != button_type) {
		env->error.code = ERROR_INVALID_ARGS;
		env->error.static_msg  =
			"Need value for new button, and button, to copy button.";
		E_SET_ERROR(*env);
		return -1;
	}
	FIAL_clear_value(argv[0], env->interp);
	argv[0]->type = button_type;
	argv[0]->ptr  = copy_button(argv[1]->ptr);

	if(!argv[0]->ptr) {
		env->error.code = ERROR_BAD_ALLOC;
		env->error.static_msg  = "couldn't allocate for new button.";
		E_SET_ERROR(*env);
		return -1;
	}
	return 0;
}

int fi_activate_button (int argc, struct FIAL_value **args,
			struct FIAL_exec_env *env, void *ptr)
{
	extern FILE *logfile;
	struct FIAL_value *ret = NULL;
	struct button     *but = NULL;

	fprintf(logfile, "activating button.....\n");

	if(argc == 0 || !args) {
		fprintf(logfile, "failed to activate, no args\n");
		return 1;
	}
	assert(args);
	if(!(ret = args[0])) {
		fprintf(logfile, "in activate button, got NULL reference\n");
		return 1;
	}

	assert(button_type);
	if(ret->type != button_type) {
		fprintf(logfile, "in activate button, reference to wrong object\n");
		fprintf(logfile, "button type : %d, ret->type : %d\n", button_type, ret->type);
		return 1;
	}

	assert(ret->ptr);

	but = ret->ptr;
	but->next = buttons;
	buttons = but;

	fprintf(logfile, "ok, button should be activated......\n");
	fprintf(logfile, "buttons = %p, ret->ptr = %p, but = %p\n",
		buttons, ret->ptr, but);

	memset(ret, 0, sizeof(*ret));
	return 0;
}

int fi_set_button_dimensions (int argc, struct FIAL_value **args,
			      struct FIAL_exec_env *env, void *ptr)
{
	float width, height;
	struct button *b = NULL;

	struct FIAL_value *val;
	if(!args || argc < 2 )
		return 1;

	if(args[0]->type != button_type)
		return 1;

	b = args[0]->ptr;
	val = args[1];

	switch(val->type) {
	case VALUE_INT:
		width = (float) val->n;
		break;
	case VALUE_FLOAT:
		width = val->x;
		break;
	default:
		width = -1.0f;
		break;
	}

	if(argc < 3) {
		height = -1.0f;
	} else {
		val = args[2];
		switch(val->type) {
		case VALUE_INT:
			height = (float) val->n;
			break;
		case VALUE_FLOAT:
			height = val->x;
			break;
		default:
			height = -1.0f;
			break;
		}
	}

	assert(b);
	if(width >= 0.0f)
		b->dim.w = width;
	if(height >= 0.0f)
		b->dim.h = height;

	return 0;

}

/*
 *  Pretty much a cut and paste from the dimensions stuff.  It is
 *  easier to just cut and paste than to use abstraction, since its
 *  just 2 things.
 *
 *  The only thing different is the bottom, where it changes position
 *  instead of dimensions.
 *
 */

int fi_set_button_position (int argc, struct FIAL_value **args,
			      struct FIAL_exec_env *env, void *ptr)
{
	float width, height;
	struct button *b = NULL;

	struct FIAL_value *val;
	if(!args || argc < 2 )
		return 1;

	if(args[0]->type != button_type)
		return 1;

	b = args[0]->ptr;
	val = args[1];

	switch(val->type) {
	case VALUE_INT:
		width = (float) val->n;
		break;
	case VALUE_FLOAT:
		width = val->x;
		break;
	default:
		width = -1.0f;
		break;
	}

	if(argc < 3) {
		height = -1.0f;
	} else {
		val = args[2];
		switch(val->type) {
		case VALUE_INT:
			height = (float) val->n;
			break;
		case VALUE_FLOAT:
			height = val->x;
			break;
		default:
			height = -1.0f;
			break;
		}
	}

	assert(b);
	if(width >= 0.0f)
		b->pos.x = width;
	if(height >= 0.0f)
		b->pos.y = height;

	return 0;

}
static float get_button_width (struct button *b)
{
	return b->dim.w;
}

static float get_button_height (struct button *b)
{
	return b->dim.h;
}

static float get_button_x (struct button *b)
{
	return b->pos.x;
}
static float get_button_y (struct button *b)
{
	return b->pos.y;
}

int fi_get_button_attribute (int argc, struct FIAL_value **argv,
				 struct FIAL_exec_env *env, void *ptr)
{
	float (*accessor) (struct button *) = ptr;

	assert(accessor);
	if(argc == 0) {
		assert(!argv);
		env->error.code = ERROR_INVALID_ARGS;
		env->error.static_msg = "Need argument to return button "
                                        "attribute.";
		E_SET_ERROR(*env);
		return -1;
	}
	assert(argv);
	if(argc < 2 || argv[1]->type != button_type) {
		env->error.code = ERROR_INVALID_ARGS;
		env->error.static_msg = "Second argument must be button to "
			                "return attribute.";
		E_SET_ERROR(*env);
		return -1;
	}

	FIAL_clear_value(argv[0], env->interp);
	argv[0]->type = VALUE_FLOAT;
	argv[0]->x = accessor(argv[1]->ptr);

	return 0;
}



struct fi_button_callback_info {
	struct FIAL_interpreter     *interp;
	struct FIAL_library            *lib;
	FIAL_symbol                     sym;
	struct FIAL_value               val;
};

int perform_fi_button_callback (struct button_event *be,
				struct button        *b,
				void               *ptr)
{
	extern FILE *logfile;

	struct fi_button_callback_info *info = ptr;
	struct FIAL_value val;
	/*set up button event arg.... */
	struct FIAL_exec_env env;
	struct FIAL_value args[2];
	int res;

	memset(&env, 0, sizeof(env));
	memset(args, 0, sizeof(args));

	fprintf(logfile, "In callback\n");

	FIAL_lookup_symbol(&val, info->lib->procs  ,  info->sym);
	fprintf(logfile, "info->interp->symbols.symbols[info->sym] : %s",
		info->interp->symbols.symbols[info->sym]);
	fprintf(logfile, "val is node: %s\n, val.ptr %p\n", val.type == VALUE_NODE ?
		"true" : "false", val.node);

	args[0].type = VALUE_REF;
	args[0].ref  = &info->val;
	args[1].type = VALUE_END_ARGS;

	env.interp = info->interp;
	env.lib    = info->lib;
	res = FIAL_run_proc(val.node, args,  &env);
	if(res < 0) {
		fprintf(logfile, "Error in %s, at line %d, col %d.\n"
			"Code %d", env.error.file, env.error.line, env.error.col,
			env.error.code);
		if(env.error.static_msg)
			fprintf(logfile, ": %s", env.error.static_msg);
		if(env.error.dyn_msg)
			fprintf(logfile, "\n %s", env.error.dyn_msg);
		fprintf(logfile, "\n");
		return -1;
	}
	return 0;
}

void destroy_callback_info (void *ptr)
{
	free (ptr);
}


int copy_callback_info (void **to, void **from)
{
	struct fi_button_callback_info *tmp = malloc(sizeof(*tmp));
	if(!tmp) {
		*to = NULL;
		return -1;
	}
	memmove(tmp, *from, sizeof(*tmp));
	*to = tmp;
	return 0;
}

int fi_set_button_callback (int argc, struct FIAL_value **args,
			    struct FIAL_exec_env *env, void *ptr)
{
	extern FILE *logfile;
	struct button                     *b;
	struct fi_button_callback_info *info;

	if(argc ==  0 || !args || args[0]->type != button_type ||
	   args[1]->type != VALUE_SYMBOL) {
		fprintf(logfile, "error, couldn't set button callback\n");
		return 1;
	}

	b = args[0]->ptr;
	info = malloc(sizeof(*info));

/* FIXME :
   when returning -1, FIAL errors should really be used....
*/
	if(!info) {
		fprintf(logfile, "couldn't allocate for button callback struct.\n");
		return -1;
	}

	memset(info, 0, sizeof(*info));
	info->interp = env->interp;
	info->lib    = env->lib;
	info->sym    = args[1]->sym;

	b->func    = perform_fi_button_callback;
	b->ptr     = info;
	b->destroy = destroy_callback_info;
	b->copy    = copy_callback_info;

	return 0;
}

static struct FIAL_c_func_def button_lib[] =
{{"create"       , fi_create_button         ,  NULL},
 {"copy"         , fi_copy_button           ,  NULL},
 {"activate"     , fi_activate_button       ,  NULL},
 {"set_dim"      , fi_set_button_dimensions ,  NULL},
 {"set_pos"      , fi_set_button_position   ,  NULL},
 {"get_h"        , fi_get_button_attribute  ,  get_button_height},
 {"get_w"        , fi_get_button_attribute  ,  get_button_width },
 {"get_x"        , fi_get_button_attribute  ,  get_button_x     },
 {"get_y"        , fi_get_button_attribute  ,  get_button_y     },
 {"set_callback" , fi_set_button_callback   ,  NULL},
 {"clear"        , fi_clear_buttons         ,  NULL},
 {NULL, NULL, NULL}};

int button_finalizer(struct FIAL_value *but, struct FIAL_interpreter *interp,
		     void *ptr)
{
	assert(but->type == button_type);
	destroy_button(but->ptr);
	return 0;
}


int fi_install_button_lib (struct FIAL_interpreter *interp,
			   struct  FIAL_c_lib     **ret_lib)
{
	extern FILE *logfile  ;
	struct FIAL_finalizer but_fin = {button_finalizer, NULL};

	/*this leaks, but that's ok for now....*/

	FIAL_register_type(&button_type, &but_fin,  interp) ;
	assert(button_type);

	fprintf(logfile, "installing button lib, button type = %d\n", button_type);

	return FIAL_load_c_lib (interp, "button", button_lib, ret_lib);
}
