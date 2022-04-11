#version 120

uniform vec3 light_positions[10];
uniform vec3 light_colors[10];
uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float s;

varying vec3 normal;
varying vec3 vert_pos;

void main()
{
	vec3 n = normalize(normal);
	vec3 cameraPos = vec3(0.0, 0.0, 0.0);
	vec3 color = ka;
	for(int i = 0; i < 10; i++) {
		vec3 l = normalize(light_positions[i]-vert_pos);
		vec3 h = normalize(normalize(cameraPos-vert_pos)+l);
		vec3 t_col = light_colors[i] * (kd*max(0, dot(l, n)) + ks*pow(max(0, dot(h, n)), s));
		float atten = 1.0 / (1.0 + 0.0429*distance(light_positions[i], vert_pos) + 0.9857*distance(light_positions[i], vert_pos)*distance(light_positions[i], vert_pos));
		color += t_col * atten;
	}
	gl_FragColor = vec4(color.rgb, 1.0);
}
