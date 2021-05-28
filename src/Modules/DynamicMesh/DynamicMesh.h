#pragma once
#include "Modules/Module.h"
#include "DirectXCollision.h"
#include "Modules/Renderer/Renderer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/IndexBuffer.hpp"

namespace RxEngine
{
    struct DynamicMeshActiveBundle
    {
        ecs::entity_t currentBundle = 0;
    };

    struct DynamicMeshVertex
    {
        DirectX::XMFLOAT3 point;
        float pad1;
        DirectX::XMFLOAT3 normal;
        float pad2;
        DirectX::XMFLOAT2 uv;
        float pad3;
        float pad4;
    };

    struct DynamicInstance
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

    struct DynamicInstanceBuffers
    {
        uint32_t count;
        std::vector<std::shared_ptr<RxCore::Buffer>> buffers;
        std::vector<std::shared_ptr<RxCore::DescriptorSet>> descriptorSets;
        std::vector<uint32_t> sizes;

        uint32_t ix;
    };

    class DynamicMeshModule : public Module
    {
    public:
        DynamicMeshModule(ecs::World * world, EngineMain * engine)
            : Module(world, engine) {}

        void startup() override;
        void shutdown() override;

    protected:
        void createOpaqueRenderCommands();
        void renderIndirectDraws(IndirectDrawSet ids,
                                 const std::shared_ptr<RxCore::SecondaryCommandBuffer> & buf) const;

    private:
        ecs::EntityHandle pipeline_;
        ecs::queryid_t worldObjects;
    };
}