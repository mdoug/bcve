#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <GL/glew.h>

#include <assert.h>

#include "bcve.h"
#include "text.h"
#include "FIAL.h"

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720

#define CONFIG_FILE "scripts/config.fial"

/* usually I'm not one for global variables, but I guess I've just
 * decided a few will make things easier, for the time being.  */

int main_argc;
char **main_argv;
const char * input_filename;
int exec_path_max = 256;
char exec_path[256];
struct window_info win_info = { NULL };

static void print_err (struct FIAL_error_info *error)
{
	printf("Error in config file %s at line %d, col %d: %s\n",
	       error->file, error->line, error->col, error->static_msg);
	if(error->dyn_msg)
		printf("Further info: %s\n", error->dyn_msg);
	exit(1);
}

static int read_config (struct FIAL_interpreter *interp,
			int *width, int *height, int *fullscreen)
{
	int max = 256;
	char buffy[256];
	int ret, sym;

	struct FIAL_error_info error;
	struct FIAL_exec_env   env;
	union  FIAL_lib_entry  *lib_ent = NULL;
	struct FIAL_value      args[4], val;

	enum{FULLSCREEN, WIDTH, HEIGHT, END};
	memset(&error, 0, sizeof(error));
	memset(&env, 0, sizeof(env));
	memset(args, 0, sizeof(args));
	memset(&val, 0, sizeof(val));

	END[args].type = VALUE_END_ARGS;

/*	FIAL_install_constants  (interp);
	FIAL_install_std_omnis  (interp);
	FIAL_install_text_buf   (interp);
	FIAL_install_system_lib (interp); */

	append_to_exec_path(buffy, CONFIG_FILE, max);
	ret = FIAL_load_string(interp, buffy,  &lib_ent, &error);

	if(ret < 0) {
		print_err(&error);
	}

	FIAL_get_symbol   (&sym, "configure"        , interp);
	FIAL_lookup_symbol(&val, lib_ent->lib.procs , sym   );

	if(val.type != VALUE_NODE) {
		printf("could find proc %s in config file\n", "configure");
		exit(1);
	}

	env.interp = interp;
	env.lib    = &lib_ent->lib;

	ret = FIAL_run_proc(val.node, args, &env);
	if(ret < 0)
		print_err(&error);

	if(FULLSCREEN[args].type == VALUE_INT)
		*fullscreen = FULLSCREEN[args].n;
	if(WIDTH[args].type == VALUE_INT)
		*width = WIDTH[args].n;
	if(HEIGHT[args].type == VALUE_INT)
		*height = HEIGHT[args].n;

	return 0;
}

static void set_exec_path (int argc, char **argv)
{
	char *ptr, *slash, *ptr2;

	/*set exec path */
	for(ptr = argv[0], slash = NULL; *ptr != '\0'; ptr++)
		if(*ptr == '\\' || *ptr == '/')
			slash = ptr;
	if(!slash)
		exec_path[0] = '\0';

	for(ptr = argv[0], ptr2 = exec_path; ptr <= slash;++ptr, ++ptr2)
		*ptr2 = *ptr;
	*ptr2 = '\0';
}

int main(int argc, char *argv[])
{
	struct FIAL_interpreter *config = NULL;
	int go, fullscreen, log;
	unsigned int window_flags = 0;

	const char *usage =
"usage: bcve [options] filename\n"
"options:\n"
"             -wWIDTH\n"
"             -hHEIGHT\n"
"             -F (for fullscreen)\n"
"             -W (for windowed)\n\b"
"options override configuration\n";

	main_argc = argc;
	main_argv = argv;

	set_exec_path(argc, argv);

	log             = 0;
	fullscreen      = 0;
	win_info.width  = SCREEN_WIDTH;
	win_info.height = SCREEN_HEIGHT;
	window_flags   |= SDL_WINDOW_OPENGL;

	config = FIAL_create_interpreter();
	read_config(config, &win_info.width, &win_info.height, &fullscreen);

	if(argc == 2) {
		input_filename = argv[1];
	} else  {
		int i;
		for(i = 1; i < argc; i++) {
			if(argv[i][0] != '-') {
				input_filename = argv[i];
				break;
			}
			switch(argv[i][1]) {
			case 'w':
				if(!isdigit(argv[i][2])) {
					puts(usage);
					exit(1);
				}
				win_info.width = strtol(argv[i] + 2, NULL, 10);
				break;
			case 'h':
				if(!isdigit(argv[i][2])) {
					puts(usage);
					exit(1);
				}
				win_info.height = strtol(argv[i] + 2, NULL, 10);
				break;
			case 'F':
				fullscreen = 1;
				break;
			case 'W':
				fullscreen = 0;
				break;
			case 'L':
				log = 1;
				break;
			default:
				puts(usage);
				exit(1);
				break;
			}
		}
		if(!input_filename) {
			puts(usage);
			exit(1);
		}
	}
	if(fullscreen)
		window_flags |= SDL_WINDOW_FULLSCREEN;

	if(log) {
		win_info.logfile = fopen("bcve.log", "w");
		if(!win_info.logfile) {
			fprintf(stderr, "Couldn't open logfile\n");
			exit(1);
		}
	} else {
		win_info.logfile = fopen("nul", "w");
	}

/*init SDL, win_info */

	SDL_Init(SDL_INIT_VIDEO);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

	win_info.window = SDL_CreateWindow("BCVE",
					   SDL_WINDOWPOS_UNDEFINED,
					   SDL_WINDOWPOS_UNDEFINED,
					   win_info.width,
					   win_info.height,
					   window_flags);
	atexit(SDL_Quit);
	if (!win_info.window) {
		printf("Could not create window: %s\n", SDL_GetError());
		return 1;
	}
	SDL_GetWindowSize(win_info.window, &win_info.width, &win_info.height);

/*init opengl and glew */

	win_info.context = SDL_GL_CreateContext(win_info.window);
	if (!win_info.context) {
		printf("There was an error creating the OpenGL context!\n");
		return 0;
	}

	const unsigned char *version = glGetString(GL_VERSION);
	fprintf(win_info.logfile, "opengl version: %s\n", version);

	glewExperimental = GL_TRUE;
	GLenum glew_status = glewInit();
	if (GLEW_OK != glew_status) {
		/* Problem: glewInit failed, something is seriously wrong. */
		fprintf(stderr, "Error: %s\n", glewGetErrorString(glew_status));
		return 0;
	}
	/*clear any errors....*/
	while((go = glGetError()) != GL_NO_ERROR) {
		fprintf(win_info.logfile, "%s\n", gluErrorString(go));
	}
	fprintf(win_info.logfile, "errors cleared...\n");


	/* init freetype */
	if(init_freetype())
	    exit(2);

	/*call my stuff! */
	/*errrm.
	  TODO: write program.
	*/
	start_viewer(&win_info);
	return 0;
}
