#version 120

uniform mat4 P;
uniform mat4 MV;
uniform mat4 T;

attribute vec4 aPos;
attribute vec3 aNor;
attribute vec2 aTex;

varying vec2 vTex0;
varying vec3 normal;
varying vec3 vert_pos;

void main()
{
	gl_Position = P * MV * aPos;
	vTex0 = aTex;
	vert_pos = (MV * aPos).xyz;
	vec4 n = vec4(aNor, 0.0);
	n = T * n;
	normal = normalize(n.xyz);
}
