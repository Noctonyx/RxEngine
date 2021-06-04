#pragma once
#include "Modules/Module.h"
#include "DirectXCollision.h"
#include "Modules/Renderer/Renderer.hpp"

namespace RxEngine
{
    struct MeshBundle
    {
        RxApi::BufferPtr vertexBuffer;
        RxApi::IndexBufferPtr indexBuffer;
        RxApi::DescriptorSetPtr descriptorSet;

        uint32_t vertexSize{};

        uint32_t vertexCount{};
        uint32_t indexCount{};

        uint32_t maxIndexCount{};
        uint32_t maxVertexCount{};

        bool useDescriptor{};

        std::vector<ecs::entity_t> entries;
    };

    struct Mesh
    {
        uint32_t vertexOffset{};
        uint32_t indexOffset{};
        uint32_t indexCount{};

        DirectX::BoundingSphere boundSphere;

        std::vector<ecs::entity_t> subMeshes;
    };

    struct SubMesh
    {
        uint32_t indexOffset;
        uint32_t indexCount;
        uint32_t subMeshIndex;
    };

    struct InBundle : ecs::Relation { };

    struct SubMeshOf : ecs::Relation { };

    struct UsesMaterial : ecs::Relation { };

    struct RenderDetailCache
    {
        ecs::entity_t bundle;
        uint32_t bundleEntry;
        ecs::entity_t shadowPipeline;
        ecs::entity_t opaquePipeline;
        ecs::entity_t transparentPipeline;
        uint32_t vertexOffset;
        uint32_t indexOffset;
        uint32_t indexCount;
        ecs::entity_t material;
        DirectX::BoundingSphere boundSphere;
    };

    class MeshModule final : public Module
    {
    public:
        MeshModule(ecs::World * world, EngineMain * engine, const ecs::entity_t moduleId)
            : Module(world, engine, moduleId) {}

        void startup() override;
        void shutdown() override;
    };
}
