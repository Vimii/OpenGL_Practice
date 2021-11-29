#shader vertex
#version 400 core
layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 v_color;
layout(location = 3) in vec2 texCoord;

out vec4 v_Color;
out vec2 v_TexCoord;
out vec3 surfaceNormal;
out vec3 toLightVector;
out vec3 toCameraVector;

uniform mat4 u_Projection;
uniform mat4 u_View;
uniform mat4 u_Model;
uniform vec3 lightPosition;

void main()
{
	vec4 worldPosition = u_Model * position;
	gl_Position = u_Projection * u_View * worldPosition;
	surfaceNormal = (u_Model * vec4(normal, 0.0)).xyz;
	toLightVector = lightPosition - worldPosition.xyz;
	toCameraVector = (inverse(u_View) * vec4(0.0, 0.0, 0.0, 1.0)).xyz - worldPosition.xyz;

	/*Pass*/
	v_Color = vec4(v_color,1.0f);
	v_TexCoord = texCoord;
}

#shader fragment
#version 400 core

in vec4 v_Color;
in vec2 v_TexCoord;
in vec3 surfaceNormal;
in vec3 toLightVector;
in vec3 toCameraVector;

layout(location = 0) out vec4 color;

uniform sampler2D u_Texture;
uniform vec4 u_Color;
uniform int bool_Tex;
uniform vec3 lightColor;
uniform float shineDamper;
uniform float reflectivity;

void main()
{
	vec3 unitNormal = normalize(surfaceNormal);
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


	if (bool_Tex == 1) {
		vec4 texColor = texture(u_Texture, v_TexCoord);
		color = texColor;
	}
	else
	{
		color = u_Color;
	}

	/*lighting*/
	vec3 light = diffuse + ambient + finalSpecular;
	color = vec4(light, 1.0) * color;
}