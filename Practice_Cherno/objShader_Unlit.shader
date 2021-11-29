#shader vertex
#version 330 core
layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 v_color;
layout(location = 3) in vec2 texCoord;

out vec4 v_Color;
out vec2 v_TexCoord;

uniform mat4 u_MVP;

void main()
{
	gl_Position = u_MVP * position;
	v_Color = vec4(v_color, 1.0f);
	v_TexCoord = texCoord;
}

#shader fragment
#version 330 core

in vec4 v_Color;
in vec2 v_TexCoord;

layout(location = 0) out vec4 color;

uniform sampler2D u_Texture;
uniform vec4 u_Color;
uniform int bool_Tex;

void main()
{
	if (bool_Tex == 1) {
		vec4 texColor = texture(u_Texture, v_TexCoord);
		color = texColor;
	}
	else
	{
		color = v_Color;
	}
}