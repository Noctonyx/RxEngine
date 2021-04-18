#version 450 core

//#extension GL_KHR_vulkan_glsl: enable

layout (set = 0, binding = 0) uniform U {
	mat4 projection;
} uboCamera;

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aUV;

layout(push_constant) uniform uPushConstant { 
    vec2 uTranslate; 
    uint textureId;
    uint pad;
    mat4 transform;
} pc;

out gl_PerVertex { vec4 gl_Position; };
layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;

void main()
{
    Out.Color = aColor;
    Out.UV = aUV;

    vec4 pos_doc = vec4(aPos + pc.uTranslate, 0, 1);
    gl_Position = uboCamera.projection * pc.transform * pos_doc;
}
