#shader vertex
#version 400 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoord;

out vec2 TexCoords;

void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
    TexCoords = texCoord;
}

#shader fragment
#version 400 core

in vec2 TexCoords;

layout(location = 0) out vec4 color;

uniform sampler2D image;

uniform bool horizontal;
uniform float weight[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
    vec2 tex_offset = 1.0 / textureSize(image, 0);
    vec3 result = texture(image, TexCoords).rgb * weight[0];
    if (horizontal)
    {
        for (int i = 1; i < 5; ++i)
        {
            result += texture(image, TexCoords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(image, TexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for (int i = 1; i < 5; ++i)
        {
            result += texture(image, TexCoords + vec2(0.0,tex_offset.y * i)).rgb * weight[i];
            result += texture(image, TexCoords - vec2(0.0,tex_offset.y * i)).rgb * weight[i];
        }
    }
    color = vec4(result,1.0);
}