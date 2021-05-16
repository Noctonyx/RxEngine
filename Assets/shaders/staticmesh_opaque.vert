#version 460

#extension GL_GOOGLE_include_directive: enable
//!#extension GL_KHR_vulkan_glsl : enable

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
};

layout(set=1, binding =0) readonly buffer V {
    Vertex vertices[];
} ;

layout(std430, set=0, binding =3) readonly buffer M {
    Material materials[];
};

layout(set=0, binding =4) uniform sampler2D textures[];

layout(set=2, binding=0) readonly buffer I {
    InstanceData instance[];
};

layout(push_constant) uniform uPushConstant {
    mat4 local; 
    uint cascadeIndex;
 } pc;

out gl_PerVertex { vec4 gl_Position; };

layout(location=0) out vec3 outPos;
layout(location=1) out vec3 outNormal;
layout(location=2) out vec2 outUv;
layout(location=3) out vec3 outViewPos;
layout(location=4) flat out uint outTexId;

void main()
{
    Vertex v = vertices[gl_VertexIndex];
    vec3 inPos = v.aPos;
    vec2 inUV = v.aUv;
    vec3 inNormal = v.aNormal;

    mat4 local = instance[gl_InstanceIndex].transform;
    uint matId = instance[gl_InstanceIndex].materialID;
    outTexId = materials[matId].colorMapIndex;

    vec4 p = uboCamera.projection * uboCamera.view * local * vec4(inPos, 1.0);

    outPos = (local * vec4(inPos,1)).xyz;
    outUv = inUV;
    gl_Position = p;

    outViewPos = (uboCamera.view * vec4(outPos, 1.0)).xyz;
    outNormal =  normalize(local * vec4(inNormal, 0.0)).xyz; 
}