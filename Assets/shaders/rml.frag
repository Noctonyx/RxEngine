#version 450 core

#extension GL_EXT_nonuniform_qualifier: enable

layout(location = 0) out vec4 fColor;

layout(set=0, binding=1) uniform sampler2D textures[];

layout(push_constant) uniform uPushConstant { 
    vec2 uTranslate; 
    uint textureId;
    uint pad;
    mat4 transform;
} pc;

layout(location = 0) in struct { vec4 Color; vec2 UV; } In;

void main()
{
    if(pc.textureId == 9999) {
        fColor = In.Color;
    } else {
        fColor = In.Color * texture(textures[pc.textureId], In.UV.st);
    }
}
