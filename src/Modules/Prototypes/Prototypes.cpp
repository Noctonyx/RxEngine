#include "Prototypes.h"

#include "imgui.h"
#include "Modules/Mesh/Mesh.h"
#include "Modules/StaticMesh/StaticMesh.h"
#include "Modules/Transforms/Transforms.h"
#include "Modules/WorldObject/WorldObject.h"
#include "sol/state.hpp"
#include "sol/table.hpp"

namespace RxEngine
{
    void visiblePrototypeUi(ecs::World * world, void * ptr)
    {
        auto visible_prototype = static_cast<VisiblePrototype*>(ptr);

        if (visible_prototype) {
            for(auto x: visible_prototype->subMeshEntities) {
                auto sm = world->get<SubMesh>(x);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Mesh Entry");
                ImGui::TableNextColumn();
                ImGui::Text("%d", sm->indexCount);
            }
        }
    }

    void PrototypesModule::startup()
    {
        world_->set<ComponentGui>(world_->getComponentId<VisiblePrototype>(), { .editor = visiblePrototypeUi });
    }

    void PrototypesModule::shutdown() { }

    void loadPrototype(ecs::World * world,
                       RxCore::Device * device,
                       const std::string & prototypeName,
                       const sol::table & details)
    {
        auto e = world->newEntity(prototypeName.c_str()).add<ecs::Prefab>();
        if(details.get_or("world_position", false)) {
            e.add<Transforms::WorldPosition>();
        }
        if (details.get_or("rotation", false)) {
            e.add<Transforms::LocalRotation>();
        }

        std::string visible = details.get<std::string>("visible");
        auto visible_entity = world->lookup(visible.c_str());
        assert(visible_entity.isAlive());

        e.set<HasVisiblePrototype>({ {visible_entity} });       
    }

    void loadPrototypes(ecs::World * world, RxCore::Device * device, sol::table & prototypes)
    {
        for (auto & [key, value]: prototypes) {
            const std::string name = key.as<std::string>();
            sol::table details = value;
            loadPrototype(world, device, name, details);
        }
    }

    void loadVisible(ecs::World* world,
        RxCore::Device* device,
        const std::string & visibleName,
        const sol::table & details)
    {
        auto e = world->newEntity(visibleName.c_str());
        auto vp = e.addAndUpdate<VisiblePrototype>();

        sol::table objects = details.get<sol::table>("objects");

        for(auto & [k, v]: objects) {
            sol::table objectDetails = v.as<sol::table>();
            std::string m = objectDetails.get<std::string>("mesh");
            uint32_t smi = objectDetails.get<uint32_t>("submesh_id");

            auto meshEntity = world->lookup(m).get<Mesh>();
            assert(meshEntity);
            assert(meshEntity->subMeshes.size() > smi);

            auto se = e.getHandle(meshEntity->subMeshes[smi]);
            vp->subMeshEntities.push_back(se);
        }
    }

    void loadVisibles(ecs::World* world, RxCore::Device* device, sol::table& visibles)
    {
        for (auto& [key, value] : visibles) {
            const std::string name = key.as<std::string>();
            sol::table details = value;
            loadVisible(world, device, name, details);
        }
    }

    void PrototypesModule::processStartupData(sol::state * lua, RxCore::Device * device)
    {
        sol::table data = lua->get<sol::table>("data");

        sol::table prototypes = data.get<sol::table>("prototypes");
        sol::table visibles = data.get<sol::table>("visibles");

        loadVisibles(world_, device, visibles);
        loadPrototypes(world_, device, prototypes);
    }
}
