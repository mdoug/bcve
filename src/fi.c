#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "FIAL.h"
#include "error_def_short.h"
#include "text_buf.h"

#include "bcve.h"

extern struct window_info *global_window_info;
extern FILE *logfile;

float offset_x = 0.0f, offset_y = 0.0f;
float scale = 1.0f;

static int print_to_log(int                           argc,
			struct FIAL_value           **args,
			struct FIAL_exec_env          *env,
			void                          *ptr)
{
	FILE *f = ptr;
	int i;

	if(argc == 0) {
		fprintf(f, "Print!  woohoo.\n");
		return 0;
	}
	assert(args);

	for(i = 0; i < argc ; i++) {
		switch(args[i]->type) {
		case VALUE_STRING:
			assert(args[i]->str);
			fprintf(f, "%s ", args[i]->str);
			break;
		case VALUE_INT:
			fprintf(f, "%d ", args[i]->n);
			break;
		case VALUE_FLOAT:
			fprintf(f, "%f ", args[i]->x);
			break;
		case VALUE_SYMBOL:
			fprintf(f, "$%s(%d) ",
				env->interp->symbols.symbols[args[i]->sym],
				args[i]->sym);
			break;
		case VALUE_TEXT_BUF:
			if(args[i]->text == NULL)
				fprintf(f, "(null text buf) ");
			else if(args[i]->text->buf == NULL)
				fprintf(f, "(empty text buf) ");
			else
				fprintf(f, "%s ", args[i]->text->buf);
			break;
		default:
			fprintf(f, "(unknown type) ");
			break;
		}
	}
	fprintf(f, "\n");
	return 0;
}

static int fi_update_offsets (int argc,  struct FIAL_value **a,
			      struct FIAL_exec_env *env, void *ptr)
{
	enum{X, Y};

	if(argc > 0) {
		switch (X[a]->type) {
		case VALUE_INT:
			offset_x += (float)X[a]->n;
			break;
		case VALUE_FLOAT:
			offset_x += X[a]->x;
			break;
		default:
			break;
		}
	}
	if(argc > 1) {
		switch (Y[a]->type) {
		case VALUE_INT:
			offset_y += (float)Y[a]->n;
			break;
		case VALUE_FLOAT:
			offset_y +=  Y[a]->x;
			break;
		default:
			break;
		}
	}
	return 0;
}

static int fi_set_scale (int argc,  struct FIAL_value **arg,
			      struct FIAL_exec_env *env, void *ptr)
{
	if(argc > 0) {
		if(arg[0]->type == VALUE_FLOAT) {
			scale = arg[0]->x;
		}
	}
	return 0;
}

static int fi_get_scale (int argc, struct FIAL_value **arg,
			 struct FIAL_exec_env *env, void *ptr)

{
	if(argc > 0) {
		FIAL_clear_value(arg[0], env->interp);
		arg[0]->type = VALUE_FLOAT;
		arg[0]->x    = scale;
	}
	return 0;
}


static int fi_get_screen_width (int                           argc,
			 struct FIAL_value           **args,
			 struct FIAL_exec_env          *env,
			 void                          *ptr)
{
	if(argc < 1) {
		return 1;
	}
	assert(args);
	FIAL_clear_value(args[0], env->interp);
	args[0]->type = VALUE_INT;
	args[0]->n    = global_window_info->width;
	return 0;
}

static int fi_get_screen_hieght (int                          argc,
				 struct FIAL_value           **args,
				 struct FIAL_exec_env          *env,
				 void                          *ptr)
{
	if(argc < 1) {
		return 1;
	}
	assert(args);
	FIAL_clear_value(args[0], env->interp);
	args[0]->type = VALUE_INT;
	args[0]->n    = global_window_info->height;
	return 0;
}

static struct FIAL_value global_map = {0};

static int fi_save (int argc, struct FIAL_value **argv,
		    struct FIAL_exec_env *env, void *ptr)
{
	struct FIAL_value none;
	memset(&none, 0, sizeof(none));

	if ( argc < 2 || argv[0]->type != VALUE_SYMBOL ) {
		env->error.code = ERROR_INVALID_ARGS;
		env->error.static_msg =
			"Saving requires: (1) symbol to save to, "
			"(2) value to save.";
		FIAL_set_error(env);
		return -1;
	}
	if(global_map.type != VALUE_MAP) {
		global_map.map = FIAL_create_symbol_map();
		if(!global_map.map) {
			env->error.code = ERROR_BAD_ALLOC;
			env->error.static_msg =
				"Couldn't allocate global symbol map.";
			FIAL_set_error(env);
			return -1;
		}
		global_map.type = VALUE_MAP;
	}
	assert(global_map.type == VALUE_MAP);
	FIAL_set_symbol(global_map.map, argv[0]->sym, argv[1], env);

	*argv[1] = none;
	return 0;
}

static int fi_load (int argc, struct FIAL_value **argv,
		    struct FIAL_exec_env *env, void *ptr)
{
	struct FIAL_value none;
	struct FIAL_symbol_map_entry *iter;
	memset(&none, 0, sizeof(none));

	if ( argc < 2 || argv[1]->type != VALUE_SYMBOL ) {
		env->error.code = ERROR_INVALID_ARGS;
		env->error.static_msg =
			"Loading requires (1) value to load to (2) symbol "
			"to load";
		FIAL_set_error(env);
		return -1;
	}
	if(global_map.type != VALUE_MAP)  {
		*argv[0] = none;
		return 1;
	}
	assert(global_map.type == VALUE_MAP);

/* bugger, I really need that copy stuff up and running, it would make
 * some of this stuff a lot easier.... */

	FIAL_clear_value(argv[0], env->interp);
	for(iter = global_map.map->first; iter != NULL; iter = iter->next){
		if(iter->sym == argv[1]->sym) {
			*argv[0] = iter->val;
			memset(&iter->val, 0, sizeof(iter->val));
			return 0;
		}
	}
	return 1;
}

static int get_arg (int argc, struct FIAL_value **argv,
	     struct FIAL_exec_env *env, void *ptr)
{
	extern int main_argc;
	extern char **main_argv;
	if(argc < 2 || argv[1]->type != VALUE_INT) {
		env->error.code = ERROR_INVALID_ARGS;
		env->error.static_msg =
			"Need a return value and an int to get an arg.";
		FIAL_set_error(env);
	}
	FIAL_clear_value (argv[0], env->interp);
	if(argv[1]->n > main_argc || argv[1]->n < 1)
		return 1;
	argv[0]->type = VALUE_STRING;
	argv[0]->str = main_argv[argv[1]->n - 1];
	return 0;
}

/* this should be a FIAL functions... */

void set_to_bad_alloc(struct FIAL_exec_env *env, char *static_msg)
{
	env->error.code = ERROR_BAD_ALLOC;
	env->error.static_msg = static_msg;
	FIAL_set_error(env);
}

/* unfortunately, I don't have an "exernal string" value or anything
 * like that, though perhaps I should think about putting one in,
 * along with some helper functions/macros to make it convenient to
 * use it */

static int get_exec_path (int argc, struct FIAL_value **argv,
			  struct FIAL_exec_env *env, void *ptr)
{
	extern char exec_path[];
	if(argc < 1) {
		return 1;
	}
	FIAL_clear_value(argv[0], env->interp);
	argv[0]->type = VALUE_TEXT_BUF;
	argv[0]->text = FIAL_create_text_buf();
	if(!argv[0]->text) {
		set_to_bad_alloc(env,
				 "couldn't allocate text buffer for exec path");
		return -1;
	}
	if(FIAL_text_buf_append_str(argv[0]->text, exec_path) < 0) {
		set_to_bad_alloc(env,
				 "couldn't make copy of exec_path");
		/*FIXME not sure if I have to free anything here or not...*/
		return -1;
	}
	return 0;

}
static int get_input_file (int argc, struct FIAL_value **argv,
			   struct FIAL_exec_env *env, void *ptr)
{
	extern const char *input_filename;

	if(argc < 1) {
		return 1;
	}
	FIAL_clear_value(argv[0], env->interp);
	argv[0]->type = VALUE_STRING;
	argv[0]->str = input_filename;
	return 0;

}

static int get_temp_dir (int argc, struct FIAL_value **argv,
			   struct FIAL_exec_env *env, void *ptr)
{
	extern const char *temp_dir;

	if(argc < 1) {
		return 1;
	}

	FIAL_clear_value(argv[0], env->interp);
	argv[0]->type = VALUE_STRING;
	argv[0]->str = temp_dir;
	return 0;

}


int install_main_fi_lib (struct FIAL_interpreter *interp)
{
	struct FIAL_c_func_def f_def[] = {
		{"print_to_log" , print_to_log         , logfile},
		{"get_screen_w" , fi_get_screen_width  , NULL},
		{"get_screen_h" , fi_get_screen_hieght , NULL},
		{"load"         , fi_load              , NULL},
		{"save"         , fi_save              , NULL},
		{"update_offsets"   , fi_update_offsets, NULL},
		{"get_scale"    , fi_get_scale         , NULL},
		{"set_scale"    , fi_set_scale         , NULL},
		{"get_arg"      , get_arg              , NULL},
		{"get_exec_path", get_exec_path        , NULL},
		{"get_input_file", get_input_file      , NULL},
		{"get_temp_dir" , get_temp_dir         , NULL},
		{NULL, NULL, NULL}
	};

	return FIAL_load_c_lib(interp, "bcve", f_def, NULL);
}
