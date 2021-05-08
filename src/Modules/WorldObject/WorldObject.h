#pragma once
#include "Modules/Module.h"
#include "DirectXMath.h"

namespace RxEngine
{
    struct WorldObject
    {
        
    };

    struct WorldTransform
    {
        DirectX::XMFLOAT4X4 transform;
    };

    class WorldObjectModule : public Module
    {
    public:
        WorldObjectModule(ecs::World* world, EngineMain* engine)
            : Module(world, engine) {}

        void startup() override;
        void shutdown() override;
    };
}
