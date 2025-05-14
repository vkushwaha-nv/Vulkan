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
    
    float angle = atan(inPos.z, inPos.x);
    float spiral = sin(dist * 1.5 - angle * 3.0 + inTime * 0.8);
    float spiralIntensity = 0.5 + 0.5 * spiral;
    
    vec3 shiftedColor;
    float angleColor = (angle + 3.14159) / (2.0 * 3.14159);
    
    shiftedColor.r = inColor.r * (0.7 + 0.3 * sin(inTime * 0.3 + angleColor * 3.14));
    shiftedColor.g = inColor.g * (0.7 + 0.3 * sin(inTime * 0.25 + dist * 0.2));
    shiftedColor.b = inColor.b * (0.7 + 0.3 * sin(angle * 1.5 + inTime * 0.2));
    
    vec3 spiralColor = mix(shiftedColor, vec3(1.0, 0.7, 0.3), spiral * 0.3 * pushConsts.colorShift);
    
    float edge = smoothstep(10.0, 20.0, dist) * spiralIntensity;
    vec3 finalColor = mix(spiralColor, vec3(0.1, 0.3, 0.8) * pushConsts.colorIntensity, edge * 0.4);
    
    finalColor = clamp(finalColor, 0.0, 1.0);
    
    outFragColor = vec4(finalColor, 1.0);
}