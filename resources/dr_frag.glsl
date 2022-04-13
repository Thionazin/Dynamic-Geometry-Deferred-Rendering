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
	gl_FragData[0].xyz = vert_pos;
	gl_FragData[1].xyz = n;
	gl_FragData[2].xyz = ka;
	gl_FragData[3].xyz = kd;
}
