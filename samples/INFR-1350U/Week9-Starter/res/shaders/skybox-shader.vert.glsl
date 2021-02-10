#version 410

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 outNormal;

uniform mat4 u_SkyboxMatrix;
<<<<<<< HEAD
uniform mat3 u_EnvironmentRotation;

void main() {
	vec4 pos = u_SkyboxMatrix * vec4(inPosition, 1.0);
	gl_Position = pos.xyww;

	// Normals
	outNormal = u_EnvironmentRotation * inPosition;
=======

void main() {
	gl_Position = vec4(inPosition.x, inPosition.y, 1, 1);

	// Normals
	outNormal = vec3(1);
>>>>>>> a21a451905e58a359e5bc28a314730632251449d
}

