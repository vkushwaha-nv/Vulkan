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

layout (binding = 1) buffer SSBO
{
    float temp1;
    float temp2;
    float temp3;
    float temp4;
} ssbo;

layout (location = 0) out vec3 outColor;


void main() 
{
    mat4 modelView = ubo.view * ubo.model;
    vec4 pos = modelView * inPos;
    outColor = inColor;
    gl_Position = ubo.projection * pos;
}
