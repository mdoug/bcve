#version 330

uniform int screen_width;
uniform int screen_height;

uniform int button_width;
uniform int button_height;

uniform int image_width;
uniform int image_height;

uniform float scale;

uniform ivec2 button_pos;
uniform vec2 screen_offsets;

smooth out vec2 my_tex_coords;
layout(location = 0) in vec2 position;

void main()
{

/* there is an error in here somewhere, not sure what it is though.
 * It's not shrinking the image to fit the screen.

 * Oh well, there is absolutely no code that is suppose to be doing
 * that.  hrmmm. Actually, I don't want to do this, because I want to
 * allow buttons to go off the screen.    */

	float sw = float(screen_width);
	float sh = float(screen_height);
	float bw = float(button_width);
	float bh = float(button_height);
	float iw = float(image_width);
	float ih = float(image_height);

	float br = bw / bh;
	float ir = iw / ih;
	float sr = sw / sh;

/*	float shrinker = min(min(1.0, sw / bw), (1.0, sh / bh)); */
	vec2 fit_scale = vec2(min(1.0, ir / br),  min(1.0, br / ir));
	vec2 tmp = position * fit_scale; /* * shrinker; */

	tmp += (vec2 (.5, .5) * (vec2(1.0 - fit_scale.x, 1.0 - fit_scale.y)));
	tmp *= vec2 (bw / sw, bh / sh);

	tmp *= 2;
	tmp -= 1.0;
	tmp += 2 * vec2(button_pos) / vec2(sw, sh);

	tmp += 2.0 * screen_offsets / vec2(sw, sh);
	tmp *= scale;

	gl_Position = vec4(tmp, 0.0, 1.0);
	my_tex_coords = position;
}
