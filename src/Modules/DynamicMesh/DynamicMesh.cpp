#include "DynamicMesh.h"

#include "Modules/Mesh/Mesh.h"
#include "Modules/WorldObject/WorldObject.h"
#include "Vulkan/ThreadResources.h"

namespace RxEngine
{
    void DynamicMeshModule::startup()
    {
        world_->addSingleton<DynamicInstanceBuffers>();

        auto sib = world_->getSingletonUpdate<DynamicInstanceBuffers>();

        sib->count = 5;
        sib->sizes.resize(5);
        sib->buffers.resize(5);
        //sib->descriptorSets.resize(5);
#if 0
        const RxCore::DescriptorPoolTemplate pool_template(
            {
                {
                    vk::DescriptorType::eStorageBuffer,
                    10
                }

            }, 10);
#endif
#if 0
        auto pl = world_->lookup("layout/general").get<PipelineLayout>();

        for (uint32_t i = 0; i < sib->count; i++) {

            sib->descriptorSets[i] = RxCore::threadResources.getDescriptorSet(
                pool_template, pl->dsls[2]);
        }
#endif
    }

    void DynamicMeshModule::shutdown() { }

    ecs::entity_t createDynamicMeshBundle(ecs::World* world)
    {
        auto mbe = world->newEntity();

        auto mb = mbe.addAndUpdate<MeshBundle>();

        mb->vertexCount = 0;
        mb->indexCount = 0;
        mb->vertexSize = sizeof(DynamicMeshVertex);
        //mb->useDescriptor = true;
        mb->maxVertexCount = (256 * 1024 * 1024 / mb->vertexSize);
        mb->maxIndexCount = mb->maxVertexCount;

        mb->vertexBuffer = RxCore::iVulkan()->createBuffer(
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer |
            vk::BufferUsageFlagBits::eTransferDst,
            VMA_MEMORY_USAGE_GPU_ONLY, mb->maxVertexCount * mb->vertexSize);

        mb->indexBuffer = RxCore::iVulkan()->createIndexBuffer(
            VMA_MEMORY_USAGE_GPU_ONLY, static_cast<uint32_t>(mb->maxVertexCount * sizeof(uint32_t)),
            false);
#if 0
        const RxCore::DescriptorPoolTemplate pool_template(
            {
                {
                    vk::DescriptorType::eStorageBuffer,
                    10
                }
            }, 10);
#endif
        //auto pl = world->lookup("layout/general").get<PipelineLayout>();
#if 0
        mb->descriptorSet = RxCore::threadResources.getDescriptorSet(pool_template, pl->dsls[1]);
        mb->descriptorSet->
            updateDescriptor(0, vk::DescriptorType::eStorageBuffer, mb->vertexBuffer);
#endif
        vk::BufferDeviceAddressInfo bdai{};
        bdai.setBuffer(mb->vertexBuffer->handle());

        mb->address = RxCore::iVulkan()->VkDevice().getBufferAddress(bdai);

        world->getSingletonUpdate<DynamicMeshActiveBundle>()->currentBundle = mbe.id;
        return mbe.id;
    }

    ecs::entity_t getActiveDynamicMeshBundle(ecs::World* world)
    {
        const auto bundle = world->getSingleton<DynamicMeshActiveBundle>();

        if (world->isAlive(bundle->currentBundle)) {
            return bundle->currentBundle;
        }

        return createDynamicMeshBundle(world);
    }
}
