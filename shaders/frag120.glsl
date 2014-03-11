#version 120

varying vec2 my_tex_coords;
uniform sampler2D texture1;
/*varying vec4 outputColor; */
void main()
{
//	outputColor = vec4(.5, .5, .5, 1.0);
	vec2 tmp = vec2 (my_tex_coords.x, 1.0 - my_tex_coords.y);
	vec4 color = texture2D(texture1, tmp);
	color *= color.a;
	gl_FragColor = color;

}
