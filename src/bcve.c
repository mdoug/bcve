#include "bcve.h"

#include <FIAL.h>
#include <text_buf.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <windows.h>
#include <process.h>

#include "text.h"

/* these are relative the the exec_path set in main -- basically
 * argv[0] without the exec name */

#define USE_OPENGL_2_1

#define VERTEX_SHADER_FILE     "shaders/vert120_2.glsl"
#define FRAGMENT_SHADER_FILE   "shaders/frag120.glsl"

/* trying out this for my text buttons.  eventually some of this stuff
   is going to have to get implemented, but of course, with shaders,
   there doesn't always seem to be one obvious way to do it.  This,
   for instance, could be essentially moved to the vertex shader, but
   then there could be difficulties surrounding multi textured
   buttons, which had different texture mappings.   */

#define TEXT_SHADER_FILE       "shaders/test_frag120_2.glsl"

#define BCVE_FIAL_FILE "scripts/bcve.fial"

/* don't cast away the const here, I need this as set, since otherwise
   the program will leak temp files */

char const *temp_dir = NULL;

char script_file[256];
extern struct button *buttons;

/* simplest for now */
struct window_info *global_window_info;

static const float button_vertices[] = {0.0f, 0.0f,
					1.0f, 0.0f,
					1.0f, 1.0f,
					0.0f, 1.0f};

static const int  button_indices[] = 	{0, 1, 2,
					 3, 0, 2};

int FIAL_install_constants (struct FIAL_interpreter *);
int FIAL_install_std_omnis (struct FIAL_interpreter *);
int install_main_fi_lib (struct FIAL_interpreter *interp);
int append_to_exec_path (char *buf, const char *str, int max_size);

int set_temp_dir (void)
{
	extern FILE *logfile;
	/*fixme: overflow checking.... */

	char buf[512];  /* this is overkill on windows, but
			   I'm not checking for overflow */
	char *tmp, *head;
	int pid;

	assert(!temp_dir);

	memset(buf, 0, sizeof(buf));
	tmp = getenv("TEMP");

	for(head = buf; *tmp != '\0'; ++tmp, ++head)
		*head = *tmp;

	*(head++) = '\\';
	for(tmp = "TMP_bcve_TMP"; *tmp != '\0'; ++tmp, ++head)
		*head = *tmp;

	CreateDirectory(buf, NULL);

	pid = _getpid();
	sprintf(head, "\\%d", pid);

	tmp = malloc(sizeof(buf));
	if(!tmp) {
		fprintf(stderr, "Couldn't allocate space to "
			"hold temp directory.\n");
		exit (10);
	}
	strcpy(tmp, buf);
	temp_dir = tmp;
	if (!CreateDirectory(buf, NULL)) {
		fprintf(logfile, "Couldn't create directory!");
		exit(29);
	} else {
		fprintf(logfile, "Created dir %s\n", buf);
	}
	return 0;
}

/* I deny any cut and pasting occurred anywhere in the vacinity of the
 * following function. */

void exec_command (char *cmd)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	ZeroMemory( &pi, sizeof(pi) );

	CreateProcess( NULL,           // No module name (use command line)
		       cmd,            // Command line
		       NULL,           // Process handle not inheritable
		       NULL,           // Thread handle not inheritable
		       FALSE,          // Set handle inheritance to FALSE
		       CREATE_NO_WINDOW,              // No creation flags
		       NULL,           // Use parent's environment block
		       NULL,           // Use parent's starting directory
		       &si,            // Pointer to STARTUPINFO structure
		       &pi );           // Pointer to PROCESS_INFORMATION structure
}

void delete_temp_dir (void)
{
	char buf[1024];
	char *head = buf;
	const char *tmp;

	memset(buf, 0, sizeof(buf));

	tmp = getenv("COMSPEC");
	for(; *tmp != '\0'; ++tmp, ++head)
		*head = *tmp;

	tmp = " /C \"rmdir /s /q \"";
	for(; *tmp != '\0'; ++tmp, ++head)
		*head = *tmp;

	tmp = temp_dir;
	for(; *tmp != '\0'; ++tmp, ++head)
		*head = *tmp;
	*(head++) = '\"';

	exec_command(buf);
}


struct FIAL_interpreter *get_FIAL_interpreter(FILE *logfile)
{
	int res;
	struct FIAL_interpreter *interp = FIAL_create_interpreter();


	FIAL_install_constants(interp);
	FIAL_install_std_omnis(interp);
	FIAL_install_system_lib(interp);
	FIAL_install_text_buf(interp);

	res = install_main_fi_lib (interp);
	assert(res >= 0);
	(void)res;

	fi_install_button_lib (interp,  NULL);

	return interp;
}

void log_FIAL_error (struct FIAL_exec_env *env)
{
	extern FILE *logfile;

	fprintf(logfile, "Error in file %s on line %d, col %d: %s.\n",
		env->error.file, env->error.line, env->error.col,
		env->error.static_msg);
	if(env->error.dyn_msg) {
		fprintf(logfile, "%s \n", env->error.dyn_msg);
	}
	return;
}

/*
 * ok, probably simplest to edit this to call the init function of
 * bcve.fial
 */

int test_call_proc (struct FIAL_interpreter *interp)
{

	extern FILE *logfile;
	FIAL_symbol sym;
	struct FIAL_value val;
	struct FIAL_library *lib = NULL;
	union FIAL_lib_entry *lib_ent = NULL;
	struct FIAL_error_info error;

	struct FIAL_exec_env  env;
	int ret;

	memset(&error, 0, sizeof(error));
	memset(&env, 0, sizeof(env));

	append_to_exec_path(script_file, BCVE_FIAL_FILE, 256);

	ret = FIAL_load_string(interp, script_file, &lib_ent, &error);
	if(ret == -1) {
		fprintf(logfile,
			"Error loading %s, at line %d, col %d:\n",
			error.file, error.line, error.col);
		if(error.static_msg) {
			fprintf(logfile, "%s\n", error.static_msg);
		}
		if(error.dyn_msg) {
			fprintf(logfile, "%s\n", error.dyn_msg);
			free(error.dyn_msg);
		}
		return -1;
	}

	FIAL_get_symbol(&sym,  "init", interp);

	lib = &lib_ent->lib;
	FIAL_lookup_symbol(&val, lib->procs, sym);

	assert((void *)lib == (void *)lib_ent);
	/* this is obsolete.....

	assert(val.type == VALUE_PROC);
	return FIAL_run_proc(interp, lib, val.proc->node);
	 */
	if(val.type != VALUE_NODE) {
		fprintf(logfile, "could find proc %s\n", "run");
		exit(1);
	}

	env.interp = interp;
	env.lib    = lib;

	perform_call_on_node(val.node, NULL, NULL, &env);
	env.skip_advance = 0;

	ret = FIAL_interpret(&env);
	if(ret < 0) {
		fprintf(logfile, "Error in file %s on line %d, col %d: %s.\n",
			env.error.file, env.error.line, env.error.col,
			env.error.static_msg);
		exit(1);
	}
	return 0;
}

void init_button_buffers(struct button_draw_stuff *bds)
{

	glGenBuffers(1, &bds->vertex_buf);
	glBindBuffer(GL_ARRAY_BUFFER, bds->vertex_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(button_vertices), button_vertices,
		     GL_STATIC_DRAW);

	glGenBuffers(1, &bds->index_buf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bds->index_buf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(button_indices),
		     button_indices, GL_STATIC_DRAW);

}

int append_to_exec_path (char *buf, const char *str, int max_size)
{
	extern char exec_path[];

	int i = 0;
	char *ptr = exec_path;

	if(max_size == 0)
		return 1;

	/* man, I'm actually using a while loop.  go me. */
	while(*ptr != '\0') {
		if(++i == max_size) {
			*buf = '\0';
			return 1;
		}
		*(buf++) = *(ptr++);
	}
	if(!str)
		return 0;
	while(*str != '\0') {
		if(++i == max_size) {
			*buf = '\0';
			return 1;
		}
		*(buf++) = *(str++);
	}
	*buf = '\0';

	return 0;
}

int init_button_gfx_program (struct button_draw_stuff *bds,
			FILE *logfile)
{
	char buffy[256];
	struct shader *vert = NULL, *frag = NULL;
	FILE *f = NULL;
	int ret = -1;

	//actually I don't know what to do about errors here, just
	//logging is hardly robust, and actually, this is openning the
	//wrong file in the event of an overrun.  But 256 is a lot of
	//characters!  well, idk...

	append_to_exec_path(buffy, VERTEX_SHADER_FILE, 256);
	f = fopen(buffy, "r");
	if(!f) {
		fprintf(logfile, "Couldn't open file for vert shader: %s\n",
			VERTEX_SHADER_FILE);
		goto cleanup;
	}
	vert = create_shader(f, SHADER_VERTEX);
	fclose(f);
	f = NULL;
	if(!vert) {
		fprintf(logfile, "unknown error -- probably allocation -- when"
			" creating vertex shader.\n");
		goto cleanup;
	}
	append_to_exec_path(buffy, FRAGMENT_SHADER_FILE, 256);
	f = fopen(buffy, "r");
	if(!f) {
		fprintf(logfile, "Couldn't open file for fragment shader: %s\n",
			FRAGMENT_SHADER_FILE);
		goto cleanup;
	}
	frag = create_shader(f, SHADER_FRAGMENT);
	if(!frag) {
		fprintf(logfile, "unknown error -- probably allocation -- when"
			" creating fragment shader.\n");
		goto cleanup;
	}
	fclose(f);
	f = NULL;

	assert(vert && frag);

	if(vert->error || frag->error)
		goto cleanup;


	bds->program = create_gfx_program();
	if(!bds->program)
		goto cleanup;

	gfx_add_shader(bds->program, vert);
	gfx_add_shader(bds->program, frag);
	return gfx_link(bds->program);

cleanup:
	if(vert) {
		if(vert->error)
			fprintf(logfile, "vertex shader error: %s\n", vert->error);
		destroy_shader(vert);
	}
	if(frag) {
		if(frag->error)
			fprintf(logfile, "fragment shader error: %s\n", frag->error);

		destroy_shader(frag);
	}
	if(f)
		fclose(f);

	return ret;

}

void init_button_locations (struct button_draw_stuff *bds)
{
	assert(bds->program->id);
	bds->screen_w_loc = glGetUniformLocation(bds->program->id, "screen_width" );
	bds->screen_h_loc = glGetUniformLocation(bds->program->id, "screen_height");
	bds->button_w_loc = glGetUniformLocation(bds->program->id, "button_width" );
	bds->button_h_loc = glGetUniformLocation(bds->program->id, "button_height");
	bds->image_w_loc  = glGetUniformLocation(bds->program->id, "image_width"  );
	bds->image_h_loc  = glGetUniformLocation(bds->program->id, "image_height" );
	bds->pos_loc      = glGetUniformLocation(bds->program->id, "button_pos"   );
	bds->offset_loc   = glGetUniformLocation(bds->program->id, "screen_offsets" );
	bds->scale_loc    = glGetUniformLocation(bds->program->id, "scale" );
}

void draw_button (struct button *b, struct button_draw_stuff *bds,
		  struct window_info *win_info)
{
	extern float offset_x, offset_y, scale;

	int b_w, b_h;
	b_w = (int) (((float)b->img->width) * (b->img->bbox.x_max - b->img->bbox.x_min));
	b_h = (int) (((float)b->img->height) * (b->img->bbox.y_max - b->img->bbox.y_min));

	glBindBuffer(GL_ARRAY_BUFFER, bds->vertex_buf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bds->index_buf);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glUseProgram(bds->program->id);

#ifdef USE_OPENGL_2_1
	glBindAttribLocation(bds->program->id, 0, "position");
#endif /*USE_OPENGL_2_1*/

	glUniform1i(bds->screen_w_loc, win_info->width);
	glUniform1i(bds->screen_h_loc, win_info->height);
	glUniform1i(bds->button_w_loc, b->dim.w);
	glUniform1i(bds->button_h_loc, b->dim.h);
	glUniform1i(bds->image_w_loc,  b_w);
	glUniform1i(bds->image_h_loc,  b_h);

	glUniform2i(bds->pos_loc, b->pos.x, b->pos.y);
	glUniform2f(bds->offset_loc, offset_x, offset_y);
	glUniform1f(bds->scale_loc, scale);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, b->img->tex);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void call_mouse_handler(SDL_MouseMotionEvent *motion,
			struct FIAL_interpreter *interp)
{
	extern FILE *logfile;
	enum {TYPE = 0, X, Y, L, R, END};
	struct FIAL_value a[END + 1];
	FIAL_symbol sym;
	int ret;

	struct FIAL_exec_env env;
	union  FIAL_lib_entry *iter;

	memset(a, 0, sizeof(a));
	memset(&env, 0, sizeof(env));

/* this is baller!  because in C x[y] is equivalent to y[x] */
	END[a].type = VALUE_END_ARGS;

	TYPE[a].type = X[a].type = Y[a].type = L[a].type = R[a].type =VALUE_INT;
	TYPE[a].n    = 1;

	X[a].n = motion->xrel;
	Y[a].n = motion->yrel;

	if(motion->state & SDL_BUTTON_LMASK)
		L[a].n = 1;
	else
		L[a].n = 0;

	if(motion->state & SDL_BUTTON_RMASK)
		R[a].n = 1;
	else
		R[a].n = 0;

	FIAL_get_symbol (&sym, "handle_mouse_move", interp);
	if(!sym) {
		fprintf(logfile, "no symbole for \"handle_mouse_move\\n");
		return;
	}

	for(iter = interp->libs ; iter != NULL; iter = iter->stub.next) {
		if(strcmp(iter->stub.label, script_file) == 0) {
			struct FIAL_library *lib = &iter->lib;
			struct FIAL_value proc;
			assert(iter->type == FIAL_LIB_FIAL);
			FIAL_lookup_symbol(&proc, lib->procs,  sym);
			assert(proc.type == VALUE_NODE);

			env.interp = interp;
			env.lib = lib;

			ret = FIAL_run_proc(proc.node, a, &env);
			if(ret < 0)
				log_FIAL_error (&env);
			return;
		}
	}
	return;

}

int init_test_image (struct image *img, struct button_draw_stuff *bds)
{
	extern FILE *logfile;
	FILE *f;
	GLuint tex;
	struct shader *vert, *frag;

	char buffy[256];
	int append_to_exec_path (char *buf, const char *str, int max_size);

	int ret;

	memset(bds, 0, sizeof(*bds));
	init_button_buffers(bds);

	if((ret = make_test_image(img))) {
		fprintf(logfile, "error making test image");
		exit(1);
	}
	tex = get_texture(img);
	if(!tex) {
		fprintf(logfile, "error getting texture");
		exit(1);
	}

	append_to_exec_path(buffy, VERTEX_SHADER_FILE, 256);
	f = fopen(buffy, "r");
	if(!f) {
		fprintf(logfile, "Couldn't open file for vert shader: %s\n",
			VERTEX_SHADER_FILE);
		goto cleanup;
	}
	vert = create_shader(f, SHADER_VERTEX);
	fclose(f);
	f = NULL;
	if(!vert) {
		fprintf(logfile, "unknown error -- probably allocation -- when"
			" creating vertex shader.\n");
		goto cleanup;
	}
	append_to_exec_path(buffy, TEXT_SHADER_FILE, 256);
	f = fopen(buffy, "r");
	if(!f) {
		fprintf(logfile, "Couldn't open file for fragment shader: %s\n",
			FRAGMENT_SHADER_FILE);
		goto cleanup;
	}
	frag = create_shader(f, SHADER_FRAGMENT);
	if(!frag) {
		fprintf(logfile, "unknown error -- probably allocation -- when"
			" creating fragment shader.\n");
		goto cleanup;
	}

	assert(vert && frag);

	if(vert->error || frag->error){
		if(vert->error)
			fprintf(logfile, "Error in vertex shader: %s", vert->error);
		if(frag->error)
			fprintf(logfile, "Error in fragment shader: %s", frag->error);
		goto cleanup;
	}


	bds->program = create_gfx_program();
	if(!bds->program)
		goto cleanup;

	gfx_add_shader(bds->program, vert);
	gfx_add_shader(bds->program, frag);
	if((ret = gfx_link(bds->program)))
		exit(1);

	init_button_locations(bds);
	return 0;

cleanup:
	exit(1);

}


void draw_test_button (struct button *b, struct button_draw_stuff *bds,
		       struct window_info *win_info)
{
    //	GLint fg_color, bg_color;
	GLint t1_bbox_loc;

	/*	fg_color = glGetUniformLocation(bds->program->id, "fg_color");
		bg_color = glGetUniformLocation(bds->program->id, "bg_color");*/
	t1_bbox_loc = glGetUniformLocation(bds->program->id, "t1_bbox");

	glUseProgram(bds->program->id);

	/* 	glUniform3f (fg_color, 1.0, 0.0, 0.0);
		glUniform3f (bg_color, 0.0, 0.0, 1.0);*/
	glUniform4f (t1_bbox_loc, b->img->bbox.x_min,  b->img->bbox.x_max,
		     b->img->bbox.y_min,  b->img->bbox.y_max);

	draw_button(b, bds, win_info);

}

void start_viewer (struct window_info *win_info)
{
	extern char **main_argv;
	extern char exec_path[];

	struct button_draw_stuff bds;
	struct FIAL_interpreter *interp;
	SDL_Event e;

	extern FILE *logfile;

	struct button test;
	struct button_draw_stuff bds_test;
	struct image img_test;

	GLuint vao;

	logfile = win_info->logfile;
	set_temp_dir();
	atexit(delete_temp_dir);
	fprintf(logfile, "%s", temp_dir);

	glGenVertexArrays (1, &vao);
	glBindVertexArray(vao);

	memset(&test, 0, sizeof(test));

	init_test_image (&img_test, &bds_test);
	test.dim.w = img_test.width;
	test.dim.h = img_test.height;
	test.img = &img_test;
	button_size_to_image(&test);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapWindow(win_info->window);

	fprintf(logfile, "argv[0] = %s\n", main_argv[0]);
	fprintf(logfile, "exec_path = %s\n", exec_path);

	global_window_info = win_info;
	interp = get_FIAL_interpreter(win_info->logfile);
	fprintf(logfile, "logfile running....\n");

	if(test_call_proc(interp) < 0 ) {
		exit (1) ;
	}

	memset(&bds,    0, sizeof(bds));
	memset(&e,      0, sizeof(e));

	init_button_buffers(&bds);
	if(init_button_gfx_program(&bds,win_info->logfile) == -1) {
		fprintf(win_info->logfile, "Error loading program, quiting.\n");
		exit (1);
	}
	if(bds.program->error) {
		fprintf(win_info->logfile, "Link error: %s\nQuitting.\n",
			bds.program->error);
		exit(1);
	}
	init_button_locations(&bds);

	if(!buttons || !win_info) {
		fprintf(win_info->logfile, "No buttons, or no win_info.  quitting.\n");
		exit(1);
	}
	fprintf(win_info->logfile, "Ready to print!  buttons:\ndim.w : %f, "
		"dim.h : %f, pos.x : %f, pos.y %f\nwin_info:\n win_info->height : %d, "
		"win_info->width : %d\n", buttons->dim.w, buttons->dim.h, buttons->pos.x,
		buttons->pos.y, win_info->height, win_info->width);

	for(;;) {
		struct button *iter;

		while(SDL_PollEvent(&e) == 1) {
			switch(e.type) {

			case SDL_MOUSEBUTTONDOWN:
			{
				float e_x = e.button.x;
				float e_y = win_info->height - e.button.y;
				struct position pos = {e_x, e_y};

				/*obviously this has to be a loop.*/
				if(button_check_position(buttons, &pos)) {
					button_handle_event(buttons, &pos);
				} else {
					fprintf(win_info->logfile, "swing and a miss!\n");
				}
			}
			break;
			case SDL_MOUSEMOTION:
				call_mouse_handler(&e, interp);
				break;
			case SDL_KEYDOWN:
			case SDL_QUIT:
			    /* clean up temp directory here */
				return;
				break;
			default:
				break;
			}
		}
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		for(iter = buttons; iter != NULL; iter = iter->next) {
			if(iter->img)
				draw_button(iter, &bds, win_info);
		}
		draw_test_button(&test, &bds_test, win_info);

		SDL_GL_SwapWindow(win_info->window);
	}
}
