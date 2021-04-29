#pragma once

#include "Modules/Module.h"

namespace RxEngine
{
    struct Prototype
    {
        
    };

    class PrototypesModule : public Module
    {
    public:
        PrototypesModule(ecs::World * world, EngineMain * engine)
            : Module(world, engine) {}

        void startup() override;
        void shutdown() override;

        void processStartupData(sol::state * lua, RxCore::Device * device) override;
    };
}
