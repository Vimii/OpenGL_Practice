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
uniform int postprocess;

void main()
{
    color = texture(screenTexture, v_TexCoord);
    color = vec4(color.x);
    //if (postprocess == 1) {
    //    float average = 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
    //    color = vec4(average, average, average, 1.0);
    //}
}