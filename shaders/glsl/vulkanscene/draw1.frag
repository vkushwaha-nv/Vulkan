#version 450

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec3 inPos;
layout (location = 2) in float inTime;

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

layout (location = 0) out vec4 outFragColor;

void main() 
{
    float dist = length(inPos);
    
    float slowTime = inTime * 0.25;
    float pulse = 0.5f + 0.4f * sin(slowTime * pushConsts.pulseSpeed);
    
    float pattern = sin(inPos.x * 5.0f) * sin(inPos.y * 5.0f) * sin(inPos.z * 5.0f);
    pattern = 0.5f + 0.3f * pattern;
    
    vec3 shiftedColor;
    shiftedColor.r = inColor.r * (0.9f + 0.2f * sin(slowTime * 0.25f + pattern * pushConsts.colorShift));
    shiftedColor.g = inColor.g * (0.9f + 0.2f * sin(slowTime * 0.2f + dist * pushConsts.colorShift * 0.15f));
    shiftedColor.b = inColor.b * (0.9f + 0.2f * cos(slowTime * 0.15f + pattern * pushConsts.colorShift));
    
    float highlight = smoothstep(15.0f, 25.0f, dist) * pulse * pushConsts.colorIntensity * 0.7f;
    
    vec3 finalColor = mix(shiftedColor, vec3(1.0f), highlight * 0.2f);
    
    finalColor *= pushConsts.colorIntensity;
    
    finalColor = clamp(finalColor, 0.0f, 1.0f);
    
    outFragColor = vec4(finalColor, 1.0f);
}