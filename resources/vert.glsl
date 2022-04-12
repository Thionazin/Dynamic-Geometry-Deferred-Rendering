#version 120

uniform mat4 P;
uniform mat4 MV;
uniform mat4 IT;
uniform float time;
attribute vec4 aPos; // In object space
attribute vec3 aNor; // In object space
attribute vec2 aTex;

varying vec3 normal; // In camera space
varying vec3 vert_pos;
varying vec2 vTex;

void main()
{
	vec3 pos_calc = vec3(aPos.x, (cos(aPos.x + time) + 2) * cos(aPos.y), (cos(aPos.x + time) + 2) * sin(aPos.y));
	gl_Position = P * (MV * vec4(pos_calc, 1.0));
	vert_pos = (MV * vec4(pos_calc, 1.0)).xyz;
	vec3 dpdx = vec3(1.0, -sin(aPos.x + time)*cos(aPos.y), -sin(aPos.x + time)*sin(aPos.y));
	vec3 dpdt = vec3(0.0, -(cos(aPos.x + time) + 2)*sin(aPos.y), (cos(aPos.x + time) + 2)*cos(aPos.y));
	vec3 nor_calc = normalize(cross(dpdt, dpdx));
	normal = normalize(vec3(IT * vec4(nor_calc, 0.0)));
	vTex = aTex;
}
