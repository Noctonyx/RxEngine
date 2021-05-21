#pragma once
#include "Modules/Module.h"
#include "DirectXCollision.h"
#include "Modules/Renderer/Renderer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/IndexBuffer.hpp"

namespace RxEngine
{
    struct StaticInstanceBuffers
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
