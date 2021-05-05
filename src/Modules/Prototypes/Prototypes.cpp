#include "Prototypes.h"

#include "Modules/StaticMesh/StaticMesh.h"
#include "Modules/Transforms/Transforms.h"
#include "sol/state.hpp"
#include "sol/table.hpp"

namespace RxEngine
{
    void PrototypesModule::startup() { }

    void PrototypesModule::shutdown() { }

    void loadPrototype(ecs::World * world,
                       RxCore::Device * device,
                       std::string prototypeName,
                       sol::table details)
    {
        auto e = world->newEntity(prototypeName.c_str()).add<ecs::Prefab>();
        if(details.get_or("world_position", false)) {
            e.add<Transforms::WorldPosition>();
        }
        if (details.get_or("y_rotation", false)) {
            e.add<Transforms::YRotation>();
        }

        sol::table mesh = details.get<sol::table>("mesh");

        if (mesh) {
            std::string m = mesh.get<std::string>(1);
            uint32_t sm = mesh.get<uint32_t>(2);

            auto meshEntity = world->lookup(m).get<StaticMesh>();
            assert(meshEntity->subMeshes.size() > sm);

            auto se = e.getHandle(meshEntity->subMeshes[sm]);
            e.set<HasSubMesh>({ {se.id} });
            e.set<Prototype>({ .boundingSphere =  meshEntity->boundSphere });
        }
    }

    void loadPrototypes(ecs::World * world, RxCore::Device * device, sol::table & prototypes)
    {
        for (auto & [key, value]: prototypes) {
            const std::string name = key.as<std::string>();
            sol::table details = value;
            loadPrototype(world, device, name, details);
        }
    }

    void PrototypesModule::processStartupData(sol::state * lua, RxCore::Device * device)
    {
        sol::table data = lua->get<sol::table>("data");

        sol::table prototypes = data.get<sol::table>("prototypes");

        loadPrototypes(world_, device, prototypes);
    }
}
