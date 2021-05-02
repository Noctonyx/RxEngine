#include "StaticMesh.h"

#include "Loader.h"
#include "Modules/Materials/Materials.h"
#include "Rendering/MeshBundle.h"
#include "sol/state.hpp"
#include "sol/table.hpp"

namespace RxEngine
{
    void StaticMeshModule::registerModule()
    {
        world_->addSingleton<StaticMeshActiveBundle>();
    }

    void StaticMeshModule::startup()
    {
        
    }

    void StaticMeshModule::shutdown() { }

    ecs::entity_t getActiveMeshBundle(ecs::World * world)
    {
        auto smab = world->getSingleton<StaticMeshActiveBundle>();
        if (world->isAlive(smab->currentBundle)) {
            return smab->currentBundle;
        }

        auto mb = std::make_shared<MeshBundle>(sizeof(StaticMeshVertex), 256 * 1024 * 1024, false);
        auto mbe = world->newEntity();
        auto smb = mbe.addAndUpdate<StaticMeshBundle>();
        smb->bundle = mb;

        world->getSingletonUpdate<StaticMeshActiveBundle>()->currentBundle = mbe.id;

        return mbe.id;
    }

    void loadMesh(ecs::World * world,
                  RxCore::Device * device,
                  std::string meshName,
                  sol::table details)
    {
        auto mbe = getActiveMeshBundle(world);

        std::string meshFile = details.get_or("mesh", std::string{""});
        auto vertices = details.get<uint32_t>("vertices");
        auto indices = details.get<uint32_t>("indices");

        std::vector<StaticMeshVertex> meshVertices(vertices);
        std::vector<uint32_t> meshIndices(indices);

        RxAssets::MeshSaveData msd;

        RxAssets::Loader::loadMesh(msd, meshFile);

        std::copy(msd.indices.begin(), msd.indices.end(), meshIndices.begin());
        std::transform(
            msd.vertices.begin(), msd.vertices.end(), meshVertices.begin(),
            [](RxAssets::MeshSaveVertex & m)
            {
                return StaticMeshVertex{
                    {m.x, m.y, m.z}, 0.f, {m.nx, m.ny, m.nz}, 0.f, {m.uvx, m.uvy}, 0.f,
                    0.f
                };
            });

        auto b = world->get<StaticMeshBundle>(mbe)->bundle;
        //if(b->canFit())

        auto me = world->newEntity(meshName.c_str()).set<MeshObject>({
            .vertexCount = vertices,
            .indexCount = indices,
            .meshFile = meshFile
        });

        auto sm = me.addAndUpdate<StaticMesh>();

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

            sm->subMeshes.push_back(
                world->newEntity()
                     //.set<ecs::InstanceOf>({{me.id}})
                     .set<MeshPrimitive>({first_index, index_count, 0})
                     .set<UsesMaterial>({{mEntities[material]}}).id
            );
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
