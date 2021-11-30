#shader vertex
#version 400 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoord;

out vec2 v_TexCoord;

void main()
{
	gl_Position = vec4(position,0.0,1.0);
	v_TexCoord = texCoord;
}

#shader fragment
#version 400 core

in vec2 v_TexCoord;

layout(location = 0) out vec4 color;

uniform sampler2D screenTexture;

void main()
{
	color = vec4(vec3(1.0 - texture(screenTexture, v_TexCoord)), 1.0);
}