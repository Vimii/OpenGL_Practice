#shader vertex
#version 400 core
layout(location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPos;
    gl_Position = projection * view * vec4(aPos, 1.0);
}

#shader fragment
#version 400 core

layout(location = 0) out vec4 color;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{
    color = texture(skybox, TexCoords);
}