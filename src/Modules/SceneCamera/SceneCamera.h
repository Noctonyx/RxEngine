#pragma once
#include "DirectXMath.h"
#include "Modules/Module.h"

namespace RxCore {
    class Buffer;
}

namespace RxEngine
{
    class Camera;

    struct SceneCameraShaderData
    {
        DirectX::XMFLOAT4X4 projection;
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT3 viewPos;
    };

    struct SceneCamera
    {
        ecs::entity_t camera;

        std::shared_ptr<RxCore::Buffer> camBuffer;
        size_t bufferAlignment;
        uint32_t ix;
        SceneCameraShaderData shaderData;

        uint32_t getDescriptorOffset() const
        {
            return static_cast<uint32_t>((ix * bufferAlignment));
        }
    };

    struct SceneCameraDescriptor
    {
        
    };

  

    class SceneCameraModule: public Module
    {
    public:
        SceneCameraModule(ecs::World * world, EngineMain * engine)
            : Module(world, engine) {}

        void startup() override;
        void shutdown() override;
    };
}
