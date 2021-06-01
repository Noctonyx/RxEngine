#pragma once

#include "Modules/Module.h"
#include "DirectXMath.h"

namespace RxCore {
    class Device;
}

namespace RxEngine
{
    struct SunDirection
    {
        DirectX::XMFLOAT3 dir;
    };

    class EnvironmentModule : public Module
    {
    public:
        EnvironmentModule(ecs::World* world, EngineMain* engine, const ecs::entity_t moduleId)
            : Module(world, engine, moduleId) {}

        void startup() override;
        void shutdown() override;
    };
}
