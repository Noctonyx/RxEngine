////////////////////////////////////////////////////////////////////////////////
// MIT License
//
// Copyright (c) 2021.  Shane Hyde (shane@noctonyx.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

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

    void DynamicMeshModule::shutdown()
    {}

    ecs::entity_t createDynamicMeshBundle(RxCore::Device * device, ecs::World * world)
    {
        auto mbe = world->newEntity();

        auto mb = mbe.addAndUpdate<MeshBundle>();

        mb->vertexCount = 0;
        mb->indexCount = 0;
        mb->vertexSize = sizeof(DynamicMeshVertex);
        //mb->useDescriptor = true;
        mb->maxVertexCount = (128 * 1024 * 1024 / mb->vertexSize);
        mb->maxIndexCount = mb->maxVertexCount;

        mb->vertexBuffer = device->createBuffer(
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer |
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            VMA_MEMORY_USAGE_GPU_ONLY, mb->maxVertexCount * mb->vertexSize
        );

        mb->indexBuffer = device->createIndexBuffer(
            VMA_MEMORY_USAGE_GPU_ONLY, static_cast<uint32_t>(mb->maxVertexCount * sizeof(uint32_t)),
            false
        );

        mb->address = mb->vertexBuffer->getDeviceAddress();

        world->getSingletonUpdate<DynamicMeshActiveBundle>()->currentBundle = mbe.id;
        return mbe.id;
    }

    ecs::entity_t getActiveDynamicMeshBundle(RxCore::Device * device, ecs::World * world)
    {
        auto bundle = world->getSingleton<DynamicMeshActiveBundle>();

        if (!bundle) {
            world->addSingleton<DynamicMeshActiveBundle>();
            bundle = world->getSingleton<DynamicMeshActiveBundle>();
        }

        if (world->isAlive(bundle->currentBundle)) {
            return bundle->currentBundle;
        }

        return createDynamicMeshBundle(device, world);
    }

    void copyToBuffers(RxCore::Device * device, const std::vector<DynamicMeshVertex> & meshVertices,
                       const std::vector<uint32_t> & meshIndices,
                       MeshBundle * smb)
    {
        size_t v_size = meshVertices.size() * sizeof(DynamicMeshVertex);
        size_t i_size = meshIndices.size() * sizeof(uint32_t);

        auto cb = device->transferCommandPool_->createTransferCommandBuffer();
        cb->begin();

        const auto b1 = device->createStagingBuffer(v_size, meshVertices.data());
        const auto b2 = device->createStagingBuffer(i_size, meshIndices.data());

        cb->copyBuffer(b1, smb->vertexBuffer, 0, smb->vertexCount * smb->vertexSize, v_size);
        cb->copyBuffer(b2, smb->indexBuffer, 0, smb->indexCount * sizeof(uint32_t), i_size);

        smb->vertexCount += static_cast<uint32_t>(meshVertices.size());
        smb->indexCount += static_cast<uint32_t>(meshIndices.size());

        cb->end();
        cb->submitAndWait();
    }

    ecs::entity_t DynamicMeshModule::createDynamicMeshObject(ecs::World * world,
                                          RxCore::Device * device,
                                          const std::vector<DynamicMeshVertex> & vertices,
                                          const std::vector<uint32_t> & indices,
                                          const std::vector<DynamicSubMeshEntry> & submeshes
    )
    {
        auto mb = getActiveDynamicMeshBundle(device, world);

        {
            auto smb = world->get<MeshBundle>(mb);

            if (vertices.size() + smb->vertexCount > smb->maxVertexCount ||
                indices.size() + smb->indexCount > smb->maxIndexCount) {
                mb = createDynamicMeshBundle(device, world);
                world->getSingletonUpdate<DynamicMeshActiveBundle>()->currentBundle = mb;
            }
        }
        auto smb = world->getUpdate<MeshBundle>(mb);

        auto dynamic_mesh_entity = world->newEntity()
                                        .set<Mesh>(
                                            {
                                                .vertexOffset = smb->vertexCount,
                                                .indexOffset = smb->indexCount,
                                                .indexCount = static_cast<uint32_t>(indices.size())
                                            }
                                        )
                                        .set<InBundle>({{mb}});

        copyToBuffers(device, vertices, indices, smb);
        smb->entries.push_back(dynamic_mesh_entity);

        auto smu = dynamic_mesh_entity.getUpdate<Mesh>();
        std::vector<ecs::entity_t> mEntities;

        uint32_t ix = 0;
        for (auto & submesh : submeshes) {
            //sm->subMeshes.push_back(
            smu->subMeshes.push_back(
                world->newEntity()
                     .set<SubMesh>({submesh.firstIndex, submesh.indexCount, ix++})
                     .set<SubMeshOf>({{dynamic_mesh_entity.id}})
                     .set<UsesMaterial>({{submesh.materialId}}).id
            );
        }

        return dynamic_mesh_entity;
    }
}
