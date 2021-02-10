#version 410

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

uniform samplerCube s_Environment;
uniform mat3 u_EnvironmentRotation;

uniform vec3  u_CamPos;

out vec4 frag_color;

// https://learnopengl.com/Advanced-Lighting/Advanced-Lighting
void main() {
<<<<<<< HEAD
	// Calculate the reflected model
	vec3 N = normalize(inNormal);
	vec3 toEye = normalize(inPos - u_CamPos);
	vec3 reflected = reflect(toEye, N);

	// Look up the environment texture
	vec3 environment = texture(s_Environment, u_EnvironmentRotation * reflected).rgb;

	// For now just return result
	frag_color = vec4(environment, 1.0);
=======
	frag_color = vec4(1, 0, 0, 1.0);
>>>>>>> a21a451905e58a359e5bc28a314730632251449d
}