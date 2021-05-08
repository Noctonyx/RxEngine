#pragma once

#include "Modules/Module.h"
#include "DirectXCollision.h"

namespace RxEngine
{
    struct VisiblePrototype
    {
        DirectX::BoundingSphere boundingSphere;
        std::vector<ecs::entity_t> subMeshEntities;
    };

    struct Prototype
    {
        //ecs::entity_t visiblePrototype;
    };

    struct HasSubMesh: ecs::Relation {};
    struct HasVisiblePrototype: ecs::Relation {};

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
