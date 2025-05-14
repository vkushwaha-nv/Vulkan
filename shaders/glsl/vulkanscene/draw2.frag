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
    
    float gridX = mod(inPos.x * 10.0, 1.0);
    float gridZ = mod(inPos.z * 10.0, 1.0);
    float grid = 1.0;
    
    if (gridX < 0.1 || gridZ < 0.1) {
        grid = 0.8;
    }
    
    float wave = sin(dist * 0.3 - inTime * 0.75);
    float pulse = 0.5 + 0.5 * wave;
    
    vec3 shiftedColor;
    shiftedColor.r = inColor.r * (0.8 + 0.3 * sin(inTime * 0.35 + dist * 0.1));
    shiftedColor.g = inColor.g * (0.8 + 0.3 * cos(inTime * 0.25 + dist * 0.15));
    shiftedColor.b = inColor.b * (0.8 + 0.3 * sin(inTime * 0.15 - dist * 0.2));
    
    shiftedColor *= grid * pushConsts.colorIntensity;
    
    float glow = smoothstep(5.0, 20.0, dist) * pulse * pushConsts.colorShift;
    vec3 finalColor = mix(shiftedColor, vec3(pulse * 0.5, pulse * 0.8, pulse), glow * 0.3);
    
    finalColor = clamp(finalColor, 0.0, 1.0);
    
    outFragColor = vec4(finalColor, 1.0);
}