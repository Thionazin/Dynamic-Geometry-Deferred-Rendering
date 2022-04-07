#version 120

uniform mat4 P;
uniform mat4 MV;
uniform mat4 IT;

attribute vec4 aPos; // in object space
attribute vec3 aNor; // in object space

varying vec3 normal;
varying vec3 vert_pos;

void main()
{
	gl_Position = P * (MV * aPos);
	vert_pos = (MV * aPos).xyz;
	vec4 n = vec4(aNor, 0.0);
	n = IT * n;
	normal = normalize(n.xyz);
}
