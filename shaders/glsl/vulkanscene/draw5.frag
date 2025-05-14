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
    
    float rings = fract(dist * 1.2 - inTime * 0.4);
    float ringEffect = smoothstep(0.0, 0.1, rings) * smoothstep(0.3, 0.2, rings);
    
    float glow = smoothstep(0.8, 0.0, abs(rings - 0.15) * 2.0) * pushConsts.colorIntensity;
    
    vec3 baseColor = inColor * (0.6 + 0.4 * sin(inTime * 0.15));
    vec3 ringColor = vec3(
        0.1 + 0.9 * sin(inTime * 0.3),
        0.2 + 0.8 * sin(inTime * 0.2 + 2.0),
        0.3 + 0.7 * sin(inTime * 0.15 + 4.0)
    );
    
    vec3 colorMix = mix(baseColor, ringColor, ringEffect * pushConsts.colorShift);
    
    vec3 finalColor = colorMix + glow * ringColor * 1.5;
    
    float edge = smoothstep(25.0, 5.0, dist);
    finalColor *= mix(0.4, 1.0, edge);
    
    finalColor = clamp(finalColor, 0.0, 1.0);
    
    outFragColor = vec4(finalColor, 1.0);
}