#version 330

//ok, this doesn't deal with location yet, just size.

uniform float screen_width;
uniform float screen_height;

uniform float button_width;
uniform float button_height;

uniform vec2 button_pos;

smooth out vec2 my_tex_coords;
layout(location = 0) in vec4 position;
layout(location = 1) in vec2 tex_coords;

void main()
{
	vec2 trans = 2 * button_pos / vec2(screen_width, screen_height);
	vec2 tmp = vec2(position.x * button_width / screen_width,
	     	        position.y * button_height / screen_height);

	tmp *= 2.0;
	tmp -= 1.0;
	tmp += trans;

	gl_Position = vec4(tmp, 0.0, 1.0);
	my_tex_coords = tex_coords;
}
