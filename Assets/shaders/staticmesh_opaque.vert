#version 460

#extension GL_GOOGLE_include_directive: enable
//#extension GL_vulkan_glsl : enable
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference_uvec2 : require

#include "lighting.glsl"

layout (set = 0, binding = 0) uniform U {
	mat4 projection;
	mat4 view;
    vec3 viewPos;
} uboCamera;

layout(set = 0, binding = 1) uniform B {
    Lighting lighting;
};

layout(set = 0, binding = 2) uniform sampler2DArray shadowMap;

struct Vertex {
    vec3 aPos;
    vec3 aNormal;
    vec2 aUv;
};

struct DrawCommand {
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int vertexOffset;
    uint firstInstance;
    //mat4 transform;
    //uint materialIndex;
};

struct InstanceData {
    mat4 transform;
    uint materialID;
};

struct Material {
    uint colorMapIndex;
    float roughness;
};

layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer ReadVertex
{
    Vertex vertices[];
};

layout(buffer_reference, std430, buffer_reference_align = 16) readonly buffer ReadInstances
{
    InstanceData instance[];
};

layout(std430, set=0, binding =3) readonly buffer M {
    Material materials[];
};

layout(set=0, binding =4) uniform sampler2D textures[];

layout(push_constant) uniform uPushConstant {
    //mat4 local; 
    //uint cascadeIndex;
    ReadVertex src;
    ReadInstances inst;
 } pc;

out gl_PerVertex { vec4 gl_Position; };

layout(location=0) out vec3 outPos;
layout(location=1) out vec3 outNormal;
layout(location=2) out vec2 outUv;
layout(location=3) out vec3 outViewPos;
layout(location=4) flat out uint outTexId;

void main()
{
    Vertex v = pc.src.vertices[gl_VertexIndex];
    vec3 inPos = v.aPos;
    vec2 inUV = v.aUv;
    vec3 inNormal = v.aNormal;

    mat4 local = pc.inst.instance[gl_InstanceIndex].transform;
    uint matId = pc.inst.instance[gl_InstanceIndex].materialID;
    outTexId = materials[matId].colorMapIndex;

    vec4 p = uboCamera.projection * uboCamera.view * local * vec4(inPos, 1.0);

    outPos = (local * vec4(inPos,1)).xyz;
    outUv = inUV;
    gl_Position = p;

    outViewPos = (uboCamera.view * vec4(outPos, 1.0)).xyz;
    outNormal =  normalize(local * vec4(inNormal, 0.0)).xyz; 
}