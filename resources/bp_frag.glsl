#version 120

uniform vec3 light_positions[10];
uniform vec3 light_colors[10];
uniform vec3 ks;
uniform float s;

uniform sampler2D pos_tex;
uniform sampler2D nor_tex;
uniform sampler2D ke_tex;
uniform sampler2D kd_tex;
uniform vec2 window_size;

void main()
{
	vec2 tex;
	tex.x = gl_FragCoord.x/window_size.x;
	tex.y = gl_FragCoord.y/window_size.y;
	vec3 position = texture2D(pos_tex, tex).rgb;
	vec3 normal = texture2D(nor_tex, tex).rgb;
	vec3 ke = texture2D(ke_tex, tex).rgb;
	vec3 kd = texture2D(kd_tex, tex).rgb;
	vec3 cameraPos = vec3(0.0, 0.0, 0.0);
	vec3 color = ke;
	if(ke == cameraPos) {
		for(int i = 0; i < 10; i++) {
			vec3 l = normalize(light_positions[i]-position);
			vec3 h = normalize(normalize(cameraPos-position)+l);
			vec3 t_col = light_colors[i] * (kd*max(0, dot(l, normal)) + ks*pow(max(0, dot(h, normal)), s));
			float atten = 1.0 / (1.0 + 0.0429*distance(light_positions[i], position) + 0.9857*distance(light_positions[i], position)*distance(light_positions[i], position));
			color += t_col * atten;
		}
	}
	// comment out this line in order to get the four images
	gl_FragColor = vec4(color.rgb, 1.0);

	// use this for positions
	// gl_FragColor.xyz = position;
	
	// use this for normals
	// gl_FragColor.xyz = normal;

	// use for emissive/ambient/whatever
	// gl_FragColor.xyz = ke;

	// use for diffuse
	// gl_FragColor.xyz = kd;
}
