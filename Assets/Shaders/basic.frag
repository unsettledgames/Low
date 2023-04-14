#version 450

layout(location = 0) in vec4 v_FragColor;
layout(location = 1) in vec2 v_TexCoord;

layout(location = 0) out vec4 OutColor;

layout(binding = 1) uniform sampler2D Texture;

void main() 
{
    OutColor = texture(Texture, v_TexCoord * 2.0);
}
