#pragma once
#include "Modules/Module.h"
#include "DirectXCollision.h"

namespace RxEngine
{
    struct MeshPrimitive
    {
        uint32_t vertexOffset;
        uint32_t indexOffset;
        uint32_t indexCount;
    };

    struct SubMesh
    {
        uint32_t firstIndex;
        uint32_t indexCount;
    };

    struct UsesMaterial : ecs::Relation
    {
        
    };

    struct StaticMesh
    {
        std::vector<ecs::entity_t> subMeshes;
        DirectX::BoundingSphere boundSphere;
    };

    struct MeshObject
    {
        uint32_t vertexCount;
        uint32_t indexCount;
        std::string meshFile;
    };

#if 0
    struct LodEntry
    {
        
    };

    struct LodDetails
    {
        std::array<LodEntry, 4> lodData;
    };

    struct SelectedLod
    {
        uint8_t selectedLod;
    };
#endif
    class StaticMeshModule : public Module
    {
    public:
        StaticMeshModule(ecs::World * world, EngineMain * engine)
            : Module(world, engine) {}

        void startup() override;
        void shutdown() override;

        void processStartupData(sol::state * lua, RxCore::Device * device) override;
    };
}
