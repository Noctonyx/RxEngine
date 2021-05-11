#pragma once
#include "Modules/Module.h"
#include "DirectXCollision.h"
#include "Modules/Renderer/Renderer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/IndexBuffer.hpp"

namespace RxEngine
{
    struct StaticMeshBundle
    {
        std::shared_ptr<RxCore::Buffer> vertexBuffer;
        std::shared_ptr<RxCore::IndexBuffer> indexBuffer;
        std::shared_ptr<RxCore::DescriptorSet> descriptorSet;

        uint32_t vertexSize;

        uint32_t vertexCount;
        uint32_t indexCount;

        uint32_t maxIndexCount;
        uint32_t maxVertexCount;

        bool useDescriptor;

        std::vector<ecs::entity_t> entries;
    };

    struct StaticMesh
    {
        uint32_t vertexOffset;
        uint32_t indexOffset;
        uint32_t indexCount;

        DirectX::BoundingSphere boundSphere;

        std::vector<ecs::entity_t> subMeshes;
    };

    struct SubMesh
    {
        uint32_t indexOffset;
        uint32_t indexCount;
    };

    struct InBundle : ecs::Relation
    {

    };

    struct SubMeshOf : ecs::Relation
    {

    };

    struct UsesMaterial : ecs::Relation
    {
        
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

//    struct StaticMeshBundle
    //{
        //std::shared_ptr<MeshBundle> bundle;
//    };

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

    struct RenderDetailCache {
        ecs::entity_t bundle;
        ecs::entity_t shadowPipeline;
        ecs::entity_t opaquePipeline;
        ecs::entity_t transparentPipeline;
        uint32_t vertexOffset;
        uint32_t indexOffset;
        uint32_t indexCount;
        ecs::entity_t material;
        DirectX::BoundingSphere boundSphere;
    };

    struct RenderingInstance
    {
        ecs::entity_t pipeline;
        ecs::entity_t bundle;
        uint32_t vertexOffset;
        uint32_t indexOffset;
        uint32_t indexCount;
        ecs::entity_t material;
        uint32_t matrixIndex;
        //DirectX::XMFLOAT4X4 mat;
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

    protected:
        void createOpaqueRenderCommands();
        void renderIndirectDraws(IndirectDrawSet ids,
                                 const std::shared_ptr<RxCore::SecondaryCommandBuffer> & buf) const;

    private:
        std::shared_ptr<RxCore::DescriptorSet> set0_;
        //std::shared_ptr<RxCore::DescriptorSet> set1;
        //std::shared_ptr<RxCore::DescriptorSet> set2;
        ecs::EntityHandle pipeline_;
        ecs::queryid_t worldObjects;
    };
}
