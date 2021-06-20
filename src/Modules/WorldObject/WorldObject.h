#pragma once
#include "Modules/Module.h"
#include "DirectXMath.h"

namespace RxEngine
{
#if 0
    struct WorldObject
    {
        
    };

    struct WorldTransform
    {
        DirectX::XMFLOAT4X4 transform;
    };

    struct DirtyTransform
    {
        bool dirty;
    };

    class WorldObjectModule : public Module
    {
    public:
        WorldObjectModule(ecs::World* world, EngineMain* engine, const ecs::entity_t moduleId)
            : Module(world, engine, moduleId) {}

        void startup() override;
        void shutdown() override;
    };
#endif
}
