#version 450

layout(binding = 0) uniform UniformBufferObject 
{
	mat4 Model;
	mat4 View;
	mat4 Projection;
} u_CameraUniforms;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in vec3 a_Normal;

layout(location = 0) out vec4 v_FragColor;
layout(location = 1) out vec2 v_TexCoord;
layout(location = 2) out vec3 v_Normal;
layout(location = 3) out vec3 v_Position;

void main() 
{
    gl_Position = u_CameraUniforms.Projection * (u_CameraUniforms.View * (u_CameraUniforms.Model * vec4(a_Position, 1.0)));
    
	v_FragColor = a_Color;
	v_TexCoord = a_TexCoord;
	v_Normal = mat3(transpose(inverse(u_CameraUniforms.Model))) * a_Normal;
	v_Position = (u_CameraUniforms.Model * vec4(a_Position, 1.0)).xyz;
}
