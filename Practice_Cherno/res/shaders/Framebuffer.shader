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

uniform sampler2D colorTexture;
uniform sampler2D depthTexture;
uniform int postprocess;

void main()
{
    float zNear = 0.0001f;
    float zFar = 1.0f;
    vec4 zbuffer = texture(depthTexture, v_TexCoord);
    float z = (2.0 * zNear * zFar) / (zFar + zNear - (zbuffer.r * 2.0 - 1.0) * (zFar - zNear));

    //color = vec4(vec3(z), 1.0);// show linear z

    //z fog
    //color = vec4(texture(colorTexture, v_TexCoord).xyz * (1.0 - z*0.5) + vec3(0.7,0.8,1.0) * z * 0.5  , 1.0) ;

    color = texture(colorTexture, v_TexCoord);
}