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
    // Create horizontal bands/stripes effect with slower movement
    float yPos = inPos.y;
    float bands = sin(yPos * 3.0 + inTime * 0.6);
    float bandsIntensity = 0.5 + 0.5 * bands;
    
    // Create slower horizontal movement effect
    float distX = length(vec2(inPos.x, inPos.z));
    float wave = sin(distX * 0.7 - inTime * 0.8);
    
    // Create layered color effect with reduced time factors
    vec3 shiftedColor;
    shiftedColor.r = inColor.r * (0.7 + 0.4 * bandsIntensity);
    shiftedColor.g = inColor.g * (0.7 + 0.4 * wave);
    shiftedColor.b = inColor.b * (0.7 + 0.4 * sin(inTime * 0.2 + yPos * 0.7));
    
    // Apply color modifiers from push constants
    vec3 modColor = mix(
        shiftedColor,
        vec3(bandsIntensity, wave * 0.5 + 0.5, sin(inTime * 0.4) * 0.5 + 0.5),
        pushConsts.colorShift * 0.3
    );
    
    // Add horizontal line highlights with slower movement
    float highlight = step(0.05, abs(fract(yPos * 1.5 + inTime * 0.1) - 0.5));
    vec3 finalColor = mix(modColor, vec3(0.8, 0.9, 1.0) * pushConsts.colorIntensity, (1.0 - highlight) * 0.2);
    
    finalColor = clamp(finalColor, 0.0, 1.0);
    
    outFragColor = vec4(finalColor, 1.0);
}