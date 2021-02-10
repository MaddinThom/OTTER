#version 410

<<<<<<< HEAD
layout(location = 0) in vec3 inNormal;

uniform samplerCube s_Environment;

out vec4 frag_color;

void main() {
	vec3 norm = normalize(inNormal);

	frag_color = vec4(texture(s_Environment, norm).rgb, 1.0);
=======
out vec4 frag_color;

void main() { 	
	frag_color = vec4(0,0,1, 1.0);
>>>>>>> a21a451905e58a359e5bc28a314730632251449d
}