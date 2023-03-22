#version 450

layout(binding = 0) uniform UniformBufferObject 
{
	mat4 Model;
	mat4 View;
	mat4 Projection;
} u_CameraUniforms;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;

layout(location = 0) out vec4 v_FragColor;

void main() 
{
    gl_Position = u_CameraUniforms.Projection * (u_CameraUniforms.View * (u_CameraUniforms.Model * vec4(a_Position, 1.0)));
    v_FragColor = a_Color;
}
