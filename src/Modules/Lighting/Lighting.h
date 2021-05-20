#pragma once
#include "DirectXCollision.h"
#include "DirectXMath.h"
#include "Modules/Module.h"

#define MAX_SHADOW_CASCADES 4

namespace RxCore
{
    class Buffer;
}

namespace RxEngine
{
    struct alignas(16) ShadowCascadeShader
    {
        DirectX::XMFLOAT4X4 viewProjMatrix;
        float splitDepth;
        DirectX::XMFLOAT3 pad_;
    };

    struct ShadowCascade
    {
        DirectX::XMFLOAT4X4 viewProjMatrix;
        DirectX::XMFLOAT4X4 viewMatrix;
        DirectX::BoundingOrientedBox boBox;
        float splitDepth;
    };

    struct LightingShaderData
    {
        float ambientStrength;
        float diffAmount;
        float specularStrength;
        float specularPower;
        DirectX::XMFLOAT3 light_direction;
        uint32_t cascadeCount;
        struct ShadowCascadeShader cascades[MAX_SHADOW_CASCADES];
    };

    struct Lighting
    {
        std::shared_ptr<RxCore::Buffer> lightingBuffer;
        size_t bufferAlignment;
        uint32_t ix;
        LightingShaderData shaderData;

        uint32_t getDescriptorOffset() const
        {
            return static_cast<uint32_t>((ix * bufferAlignment));
        }
    };

    struct LightingDescriptor
    {

    };

    class LightingModule : public Module
    {
    public:
        LightingModule(ecs::World * world, EngineMain * engine)
            : Module(world, engine) {}

        void startup() override;
        void shutdown() override;
    };
}