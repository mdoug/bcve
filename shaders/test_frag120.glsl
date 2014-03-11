
varying vec2 my_tex_coords;
uniform sampler2D texture1;

void main()
{
	vec3 fg_color = vec3(0.0, 0.0, 1.0);
	vec3 bg_color = vec3(0.0, 0.5, 0.5);

	vec2 tmp = vec2 (my_tex_coords.x, my_tex_coords.y);
	float red = texture2D(texture1, tmp).r;

	vec3 final_color = (fg_color * red) + (bg_color * (1.0 - red));

	gl_FragColor = vec4(final_color,  1.0);
}
