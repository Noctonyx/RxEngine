#pragma once
#include "Modules/Module.h"
#include "DirectXCollision.h"

namespace RxEngine
{
    class MeshBundle;

    struct MeshPrimitive
    {
        uint32_t vertexOffset;
        uint32_t indexOffset;
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

    struct StaticMeshActiveBundle
    {
        ecs::entity_t currentBundle = 0;
    };

    struct StaticMeshBundle
    {
        std::shared_ptr<MeshBundle> bundle;
    };

    struct StaticMeshVertex
    {
        DirectX::XMFLOAT3 point;
        float pad1;
        DirectX::XMFLOAT3 normal;
        float pad2;
        DirectX::XMFLOAT2 uv;
        float pad3;
        float pad4;
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

        void registerModule() override;
        void startup() override;
        void shutdown() override;

        void processStartupData(sol::state * lua, RxCore::Device * device) override;
    };
}
