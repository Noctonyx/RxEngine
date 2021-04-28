#pragma once
#include "Modules/Module.h"
#include "DirectXCollision.h"

namespace RxEngine
{
    struct WorldObject
    {
        
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
