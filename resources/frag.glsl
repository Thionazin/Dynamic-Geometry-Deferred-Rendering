#version 120

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform vec3 lightPosCam;

varying vec2 vTex0;
varying vec3 normal;
varying vec3 vert_pos;

void main()
{
	vec3 n = normalize(normal);
	vec3 cameraPos = vec3(0.0, 0.0, 0.0);
	vec3 l = normalize(lightPosCam-vert_pos);
	vec3 h = normalize(normalize(cameraPos-vert_pos)+l);
	vec3 lcol = vec3(0.8, 0.8, 0.8);
	vec3 kd = texture2D(texture0, vTex0).rgb;
	vec3 ks = texture2D(texture1, vTex0).rgb;
	vec3 color = lcol*(kd*max(0, dot(l, n)) + ks*pow(max(0, dot(h, n)), 50));
	gl_FragColor = vec4(color, 1.0);
}
