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

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

uniform sampler2D colorTexture;
uniform float threshold;


void main()
{

    FragColor = texture(colorTexture, v_TexCoord);
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > threshold)
        BrightColor = vec4(FragColor.xyz * brightness,1.0);
    else if (brightness > threshold / 2.0)
        BrightColor = vec4(FragColor.xyz * threshold * (1.0 - (threshold - brightness) / (threshold / 2.0)),1.0);
    else
        BrightColor = vec4(vec3(0.0), 1.0);
}