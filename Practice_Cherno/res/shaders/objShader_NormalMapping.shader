#shader vertex
#version 400 core

/*–¢ŽÀ‘•*/

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 v_color;
layout(location = 3) in vec2 texCoord;

out vec4 v_Color;
out vec2 v_TexCoord;
out vec3 surfaceNormal;
out vec3 toLightVector;
out vec3 toCameraVector;
out vec3 ReflectNormal;
out vec3 ReflectPosition;
out vec3 cameraPos;

uniform mat4 u_Projection;
uniform mat4 u_View;
uniform mat4 u_Model;
uniform vec3 lightPosition;

void main()
{
	cameraPos = (inverse(u_View) * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
	vec4 worldPosition = u_Model * position;

	surfaceNormal = (u_Model * vec4(normal, 0.0)).xyz;
	toLightVector = lightPosition - worldPosition.xyz;
	toCameraVector = cameraPos - worldPosition.xyz;

	ReflectNormal = mat3(transpose(inverse(u_Model))) * normal;
	ReflectPosition = vec3(u_Model * vec4(vec3(position),1.0));

	/*Pass*/
	v_Color = vec4(v_color,1.0f);
	v_TexCoord = texCoord;

	gl_Position = u_Projection * u_View * worldPosition;
}

#shader fragment
#version 400 core

in vec4 v_Color;
in vec2 v_TexCoord;
in vec3 surfaceNormal;
in vec3 toLightVector;
in vec3 toCameraVector;
in vec3 ReflectNormal;
in vec3 ReflectPosition;
in vec3 cameraPos;

layout(location = 0) out vec4 color;

uniform sampler2D u_Texture_Diffuse;
uniform sampler2D u_Texture_Specular;
uniform sampler2D u_Texture_Normal;
uniform vec4 u_Color;
uniform int bool_Tex_Dif,bool_Tex_Spec;
uniform vec3 lightColor;
uniform float shineDamper;
uniform float reflectivity;
uniform samplerCube skybox;

void main()
{
	//vec3 unitNormal = normalize(surfaceNormal);
	vec3 unitNormal = normalize(texture(u_Texture_Normal, v_TexCoord).rgb) * 2.0 - 1.0);
	vec3 unitLightVector = normalize(toLightVector);

	float nDot1 = dot(unitNormal, unitLightVector);
	float brightness = max(nDot1, 0.0);
	vec3 diffuse = brightness * lightColor;

	float ambientStrength = 0.1;
	vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);

	vec3 unitVectorToCamera = normalize(toCameraVector);
	vec3 lightDirection = -unitLightVector;
	vec3 reflectedLightDirection = reflect(lightDirection, unitNormal);

	float specularFactor = dot(reflectedLightDirection, unitVectorToCamera);
	specularFactor = max(specularFactor, 0.0);
	float dampedFactor = pow(specularFactor, shineDamper);
	vec3 finalSpecular = dampedFactor * reflectivity * lightColor;
	if (bool_Tex_Spec == 1)
		finalSpecular *= vec3(texture(u_Texture_Specular, v_TexCoord));

	if (bool_Tex_Dif == 1) {
		vec4 texColor = texture(u_Texture_Diffuse, v_TexCoord);
		color = texColor;
	}
	else
	{
		color = v_Color;
	}

	/*SkyReflection*/
	vec3 I = normalize(ReflectPosition - cameraPos);
	vec3 R = reflect(I, normalize(ReflectNormal));
	vec4 ReflectColor = vec4(texture(skybox, R).rgb, 1.0);
	color = reflectivity * ReflectColor + (1.0 - reflectivity) * color;

	/*lighting*/
	vec3 light = diffuse + ambient + finalSpecular;
	color = vec4(light, 1.0) * color;
	//color = ReflectColor;
}