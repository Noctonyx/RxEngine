#version 460

#extension GL_GOOGLE_include_directive: enable

#define ambient 0.5
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

//layout(std430, set=1, binding =1) readonly buffer M {
    //Material materials[];
//};

layout(set=0, binding =4) uniform sampler2D textures[];

layout(set=2, binding=0) readonly buffer I {
    InstanceData instance[];
};

layout(push_constant) uniform uPushConstant {
    mat4 local; 
    uint cascadeIndex;
 } pc;

layout(location=0) in vec3 inPos;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inUv;
layout(location=3) in vec3 inViewPos;

layout(location = 0) out vec4 outFragColor;

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 
);

float textureProj(vec4 shadowCoord, vec2 offset, uint cascadeIndex)
{
	float shadow = 1.0;
	float bias = 0.0005;

	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) {
		float dist = texture(shadowMap, vec3(shadowCoord.st + offset, cascadeIndex)).r;
		if (shadowCoord.w > 0 && dist < shadowCoord.z - bias) {
			shadow = ambient;
		}
	}
	return shadow;
}

float filterPCF(vec4 sc, uint cascadeIndex)
{
	ivec2 texDim = textureSize(shadowMap, 0).xy;
	float scale = 0.75;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 2;
	
	for (int x = -range; x <= range; x++) {
		for (int y = -range; y <= range; y++) {
			shadowFactor += textureProj(sc, vec2(dx*x, dy*y), cascadeIndex);
			count++;
		}
	}
	return shadowFactor / count;
}

void main() {

    float shadow = 1.0;
	float bias = 0.005;

    vec4 color = texture(textures[1], inUv);
        //color = vec4(1.0f);
    uint cascadeIndex = 0;

    for(uint i=0; i < lighting.cascadeCount - 1; ++i) {
        if(inViewPos.z < lighting.cascades[i].splitDepth) {
            cascadeIndex = i + 1;
        }
    }

    vec4 shadowCoord = (biasMat * lighting.cascades[cascadeIndex].viewProjMatrix) * vec4(inPos, 1.0);
    //shadow = textureProj(shadowCoord / shadowCoord.w, vec2(0.0), cascadeIndex);
    shadow = filterPCF(shadowCoord / shadowCoord.w, cascadeIndex);
    vec3 N = normalize(inNormal);
	vec3 L = normalize(-lighting.light_direction);
	vec3 H = normalize(L + inViewPos);
	float diffuse = max(dot(N, L), ambient);
	vec3 lightColor = vec3(1.0);

	outFragColor.rgb = max(lightColor * (diffuse * color.rgb), vec3(0.0));
	outFragColor.rgb *= shadow;
	outFragColor.a = color.a;
}
