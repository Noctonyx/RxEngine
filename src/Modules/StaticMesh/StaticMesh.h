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
    };

    struct StaticMesh
    {
        std::vector<ecs::entity_t> subMeshes;
        DirectX::BoundingSphere boundSphere;
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
    };
}
