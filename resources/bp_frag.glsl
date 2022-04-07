#version 120

uniform vec3 lightPos;
uniform vec3 lcol;
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
	vec3 l = normalize(lightPos-vert_pos);
	vec3 h = normalize(normalize(cameraPos-vert_pos)+l);
	vec3 color = lcol*(ka + kd*max(0, dot(l, n)) + ks*pow(max(0, dot(h, n)), s));
	gl_FragColor = vec4(color.rgb, 1.0);
}
