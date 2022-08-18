#shader vertex
#version 400 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoord;

out vec2 v_TexCoord;

void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
    v_TexCoord = texCoord;
}

#shader fragment
#version 400 core

in vec2 v_TexCoord;

layout(location = 0) out vec4 color;

uniform sampler2D screenTexture;
uniform sampler2D accumTexture;
uniform int sampleNum;
uniform int first;

void main()
{
    color = texture(accumTexture, v_TexCoord);
    if (first == 0) color = color / sampleNum;
    else
    {
        color = color + texture(screenTexture, v_TexCoord) / sampleNum;
    }
    color = vec4(color.xyz, 1.0);
    
}