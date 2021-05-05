#pragma once

#include "Modules/Module.h"
#include "DirectXCollision.h"

namespace RxEngine
{
    struct Prototype
    {
        DirectX::BoundingSphere boundingSphere;
    };

    struct HasSubMesh: ecs::Relation {};

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
