#version 120

uniform mat4 P;
uniform mat4 MV;

attribute vec4 aPos; // in object space
//attribute vec3 aNor; // in object space

varying vec3 normal;
varying vec3 vert_pos;

void main()
{
	gl_Position = P * (MV * aPos);
}
