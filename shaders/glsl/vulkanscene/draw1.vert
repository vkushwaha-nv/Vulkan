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

// Add storage buffer binding that may not exist
layout(binding = 1) buffer SSBO {
    vec4 data[];
} ssbo;

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
    
    vec3 enhancedColor = inColor * (0.8 + 0.2 * pushConsts.colorMod);
    
    // Check if we should crash
    if (pushConsts.crashType == 1) {
        // Access out-of-bounds data to cause a memory fault
        int invalidIndex = int(pushConsts.crashValue1) + int(pushConsts.crashValue2);
        vec4 invalidData = ssbo.data[invalidIndex];
        outColor = enhancedColor * invalidData.xyz;
    }
    
    else if (pushConsts.crashType == 2) {
        // Simple division by zero crash
        float result = float(pushConsts.crashValue1) / float(pushConsts.crashValue2);
        outColor = enhancedColor * result;
    } 
    
    else if (pushConsts.crashType == 3) {
        // Simple infinite loop
        outColor = enhancedColor;
        
        while (pushConsts.crashValue1 != pushConsts.crashValue2) {
            // Keep changing the color in the loop
            outColor = outColor * 0.99 + enhancedColor * 0.01;
        }
    }
    else {
        // normal behavior
        outColor = enhancedColor;
    }
    
    outTime = pushConsts.time * 0.15 + pushConsts.animationTime * pushConsts.pulseSpeed * 0.2;
    
    gl_Position = ubo.projection * pos;
}
