#version 330

in vec2 my_tex_coords;
uniform sampler2D texture1;
out vec4 outputColor;
void main()
{
//	outputColor = vec4(.5, .5, .5, 1.0);
	vec2 tmp = vec2 (my_tex_coords.x, 1.0 - my_tex_coords.y);
	vec4 color = texture(texture1, tmp);
	color *= color.a;
	outputColor = color;

}
