#version 450

layout(location = 0) in vec4 v_FragColor;
layout(location = 1) in vec2 v_TexCoord;
layout(location = 2) in vec3 v_Normal;
layout(location = 3) in vec3 v_Position;

layout(location = 0) out vec4 OutColor;

// TODO: Push constants
layout(binding = 1) uniform sampler2D u_Texture;

layout(push_constant) uniform Consts
{
	vec3 CameraPosition;
	float Metallic;
	float Roughness;
	float AO;
} u_PushConsts;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

void main() 
{
	vec3 cameraPos = -u_PushConsts.CameraPosition;

	vec3 N = normalize(v_Normal);
    vec3 V = normalize(cameraPos - v_Position);
	vec3 lightPositions[1];
	vec3 lightColors[1];
	
	
	lightPositions[0] = vec3(4, 4, 3);
	lightColors[0] = vec3(23.47, 21.31, 20.79);

    vec3 F0 = vec3(0.04); 
	vec3 albedo = vec3(1.0);//texture(u_Texture, v_TexCoord).xyz;
    F0 = mix(F0, albedo, u_PushConsts.Metallic);
	
    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 1; i++) 
    {
        // calculate per-light radiance
        vec3 L = normalize(lightPositions[i] - v_Position);
        vec3 H = normalize(V + L);
        float distance    = length(lightPositions[i] - v_Position);
        float attenuation = 1.0 / (distance);
        vec3 radiance     = lightColors[i] * attenuation;        
        
        // cook-torrance brdf
        float NDF = DistributionGGX(N, H, u_PushConsts.Roughness);
        float G   = GeometrySmith(N, V, L, u_PushConsts.Roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - u_PushConsts.Metallic;	  
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;  
            
        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
    }   
  
    vec3 ambient = vec3(0.03) * albedo * u_PushConsts.AO;
    vec3 color = ambient + Lo;
	
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  
   
   // Phong: vec3(1.0) * dot(normalize(N), normalize(lightPositions[0] - v_Position))
    OutColor = vec4(color, 1.0);
}