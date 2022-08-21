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
uniform sampler2D depthTexture;
uniform float focusDistance;
uniform float blurRadius;


//uniform float weight[25] = float[](1.0, 4.0, 6.0, 4.0, 1.0, 4.0, 16.0, 24.0, 16.0, 4.0, 6.0, 24.0, 36.0, 24.0, 6.0, 4.0, 16.0, 24.0, 16.0, 4.0, 1.0, 4.0, 6.0, 4.0, 1.0);

uniform float weight[49] = float[](1, 6, 15, 20, 15, 6, 1, 6, 36, 90, 120, 90, 36, 6,  15, 90, 225, 300, 225, 90, 15, 20, 120, 300, 400, 300, 120, 20, 15, 90, 225, 300, 225, 90, 15, 6, 36, 90, 120, 90, 36, 6, 1, 6, 15, 20, 15, 6, 1);


void main()
{
    float zNear = 0.0001f;
    float zFar = 1.0f;
    vec4 zbuffer = texture(depthTexture, TexCoords);
    float z = (2.0 * zNear * zFar) / (zFar + zNear - (zbuffer.r * 2.0 - 1.0) * (zFar - zNear));

    color = vec4(vec3(0.0), 1.0);

    float rate = abs(z - focusDistance);
    float _TexelInterval = blurRadius * rate;
    vec2 tex_offset = 1.0 / textureSize(image, 0);

    for (int i = 0; i < 7; i++)
    {
        for (int j = 0; j < 7; j++)
        {
            vec2 offset = vec2(tex_offset.x * (j - 3), tex_offset.y * (i-3)) * _TexelInterval;
            color.rgb += texture(image, TexCoords + offset).xyz * weight[i * 7 + j] / 4096.0;
        }
    }

}