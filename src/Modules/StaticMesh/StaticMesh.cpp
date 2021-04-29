#include "StaticMesh.h"

#include "Modules/Materials/Materials.h"
#include "sol/state.hpp"
#include "sol/table.hpp"

namespace RxEngine
{
    void StaticMeshModule::startup() { }

    void StaticMeshModule::shutdown() { }

    void loadMesh(ecs::World * world,
                  RxCore::Device * device,
                  std::string meshName,
                  sol::table details)
    {
        std::string meshFile = details.get_or("mesh", std::string{""});
        auto vertices = details.get<uint32_t>("vertices");
        auto indices = details.get<uint32_t>("indices");

        auto me = world->newEntity(meshName.c_str()).set<MeshObject>({
            .vertexCount = vertices,
            .indexCount = indices,
            .meshFile = meshFile
        });

        sol::table smtab = details.get<sol::table>("submeshes");
        sol::table mtab = details.get<sol::table>("materials");

        std::vector<ecs::entity_t> mEntities;

        for (auto & [k, v]: mtab) {
            std::string mn = v.as<std::string>();

            auto mat_entity = world->lookup(mn.c_str());
            if (mat_entity.isAlive() && mat_entity.has<Material>()) {
                mEntities.push_back(mat_entity.id);
            }
        }

        for (auto & [k, v]: smtab) {
            sol::table subMeshValue = v;

            uint32_t first_index = subMeshValue.get<uint32_t>("first_index");
            uint32_t index_count = subMeshValue.get<uint32_t>("index_count");
            const uint32_t material = subMeshValue.get<uint32_t>("material");

            world->newEntity()
                 .set<ecs::InstanceOf>({{me.id}})
                 .set<SubMesh>({first_index, index_count})
                 .set<UsesMaterial>({{mEntities[material]}});
        }
    }

    void loadMeshes(ecs::World * world, RxCore::Device * device, sol::table & meshes)
    {
        for (auto & [key, value]: meshes) {
            const std::string name = key.as<std::string>();
            const sol::table details = value;
            loadMesh(world, device, name, details);
        }
    }

    void StaticMeshModule::processStartupData(sol::state * lua, RxCore::Device * device)
    {
        const sol::table data = lua->get<sol::table>("data");

        sol::table meshes = data.get<sol::table>("meshes");

        loadMeshes(world_, device, meshes);
    }
}
