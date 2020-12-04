#version 410

// THIS FILE IS JUST THE FIRST ATTEMPT AT THE TERRAIN FRAG BUT IT GOT MESSED UP SOMEHOW
// I KEPT IT JUST IN CASE

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

uniform sampler2D s_Sand;
uniform sampler2D s_Grass;
uniform sampler2D s_Rock;
uniform sampler2D s_Snow;

uniform vec3 u_HeightCutoffs;
uniform vec3 u_InterpolateFactors;

uniform vec3  u_AmbientCol;
uniform float u_AmbientStrength;

uniform vec3  u_LightPos;
uniform vec3  u_LightCol;
uniform float u_AmbientLightStrength;
uniform float u_SpecularLightStrength;
uniform float u_Shininess;
// NEW in week 7, see https://learnopengl.com/Lighting/Light-casters for a good reference on how this all works, or
// https://developer.valvesoftware.com/wiki/Constant-Linear-Quadratic_Falloff
uniform float u_LightAttenuationConstant;
uniform float u_LightAttenuationLinear;
uniform float u_LightAttenuationQuadratic;

uniform float u_TextureMix;

uniform vec3  u_CamPos;

out vec4 frag_color;

// https://learnopengl.com/Advanced-Lighting/Advanced-Lighting
void main() {
	// Lecture 5
	vec3 ambient = u_AmbientLightStrength * u_LightCol;

	// Diffuse
	vec3 N = normalize(inNormal);
	vec3 lightDir = normalize(u_LightPos - inPos);

	float dif = max(dot(N, lightDir), 0.0);
	vec3 diffuse = dif * u_LightCol;// add diffuse intensity

	//Attenuation
	float dist = length(u_LightPos - inPos);
	float attenuation = 1.0f / (
		u_LightAttenuationConstant + 
		u_LightAttenuationLinear * dist +
		u_LightAttenuationQuadratic * dist * dist);

	// Specular
	vec3 viewDir  = normalize(u_CamPos - inPos);
	vec3 h        = normalize(lightDir + viewDir);

	// Get the specular power from the specular map
	float spec = pow(max(dot(N, h), 0.0), u_Shininess); // Shininess coefficient (can be a uniform)
	vec3 specular = u_SpecularLightStrength * * spec * u_LightCol; // Can also use a specular color

	float height = inUV.x;

	vec3 upperEdge = u_HeightCutoffs + u_InterpolateFactors;
	vec3 lowerEdge = u_HeightCutoffs - u_InterpolateFactors;

	vec4 texPowers = vec4(
		smoothstep(height, upperEdge.x, lowerEdge.x),
		min(smoothstep(height, lowerEdge.x, upperEdge.x), smoothstep(height, upperEdge.y, lowerEdge.y)),
		min(smoothstep(height, lowerEdge.y, upperEdge.y), smoothstep(height, upperEdge.z, lowerEdge.z)),
		smoothstep(height, lowerEdge.z, upperEdge.z)
	);

	// Get the albedo from the diffuse / albedo map
	vec4 sand = texture(s_Sand, inUV);
	vec4 grass = texture(s_Grass, inUV);
	vec4 rock = texture(s_Rock, inUV);
	vec4 snow = texture(s_Snow, inUV);

	vec4 textureColor = 
		sand * texPowers.x +
		grass * texPowers.y +
		rock * texPowers.z +
		snow * texPowers.w;

	vec3 result = (
		(u_AmbientCol * u_AmbientStrength) + // global ambient light
		(ambient + diffuse + specular) * attenuation // light factors from our single light
		) * inColor * textureColor.rgb; // Object color

	frag_color = vec4(result, textureColor.a);
}