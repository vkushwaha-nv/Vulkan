#version 450
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference_uvec2 : require

layout (location = 0) in vec3 inColor;

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform PushConstants {
   uint addressHi;
   uint addressLo;
} pushConstants;

layout(buffer_reference) buffer VKKK;
layout(buffer_reference, buffer_reference_align = 4, std430) buffer VKKK
{
    uint m0[];
};

void main() 
{
    if (pushConstants.addressLo == 0xFED000) {
        uvec2 address = uvec2(0x0, 0x0);
        VKKK(address).m0[0] += 0x1234;
    }

    outFragColor = vec4(inColor, 0.6);
}