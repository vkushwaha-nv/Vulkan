#version 450

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec3 inColor;

layout (binding = 0) uniform UBO 
{
    mat4 projection;
    mat4 model;
    mat4 normal;
    mat4 view;
    vec3 lightpos;
} ubo;

layout(push_constant) uniform PushConsts {
    float time;
    float animationTime;
    float colorMod;
    float colorShift;
    float pulseSpeed;
    float colorIntensity;
    uint crashType;
    uint crashValue1;
    uint crashValue2;
} pushConsts;

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec3 outPos;
layout (location = 2) out float outTime;

void main() 
{
    mat4 modelView = ubo.view * ubo.model;
    vec4 pos = modelView * inPos;
    
    outPos = inPos.xyz;
    
    float heightFactor = (inPos.y + 15.0) / 30.0;
    float heightWave = sin(heightFactor * 8.0 + pushConsts.time * 0.2);
    vec3 enhancedColor = inColor * (0.6 + 0.5 * heightWave * pushConsts.colorMod);
    outColor = enhancedColor;
    
    outTime = pushConsts.time * 0.15 + heightFactor * pushConsts.pulseSpeed * 0.3;
    
    gl_Position = ubo.projection * pos;
}