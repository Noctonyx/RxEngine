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

#include <Modules/RTSCamera/RTSCamera.h>
#include <Modules/SceneCamera/SceneCamera.h>
#include <Modules/Transforms/Transforms.h>
#include "DynamicMesh.h"

#include "Modules/Render.h"
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

        worldObjects_ = world_->createQuery()
                              .with<WorldObject, WorldTransform, DynamicMesh,
                                  Transforms::LocalBoundingSphere>()
                              .withJob()
                              .withInheritance(true).id;

        world_->createSystem("DynamicMesh:Render")
              .inGroup("Pipeline:Render")
              .withStreamWrite<Render::OpaqueRenderCommand>()
              .withRead<CurrentMainDescriptorSet>()
              .withRead<DescriptorSet>()
              .withRead<PipelineLayout>()
              .withRead<DynamicMesh>()
              .withJob()
              .execute(
                  [this](ecs::World *) {
                      OPTICK_EVENT("DynamicMesh:Render")
                      createOpaqueRenderCommands();
                  }
              );
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
        mb->maxVertexCount = (64 * 1024 * 1024 / mb->vertexSize);
        mb->maxIndexCount = mb->maxVertexCount * 3 / 2;

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

    void copyToBuffers(RxCore::Device * device,
                       const std::vector<DynamicMeshVertex> & meshVertices,
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

    ecs::EntityHandle DynamicMeshModule::createDynamicMeshObject(
        ecs::World * world,
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
                                        .add<DynamicMesh>()
                                        .set<InBundle>({{mb}});

        copyToBuffers(device, vertices, indices, smb);
        smb->entries.push_back(dynamic_mesh_entity);

        auto smu = dynamic_mesh_entity.getUpdate<Mesh>();
        std::vector<ecs::entity_t> mEntities;

        uint32_t ix = 0;
        for (auto & submesh: submeshes) {
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

    void DynamicMeshModule::createOpaqueRenderCommands()
    {
        OPTICK_CATEGORY("Render Dynamic", ::Optick::Category::Rendering)

        if (!pipeline_.isAlive()) {
            pipeline_ = world_->lookup("pipeline/staticmesh_opaque");
        }
        auto pipeline = pipeline_.get<GraphicsPipeline>();

        if (!pipeline) {
            return;
        }

        assert(pipeline);
        assert(pipeline->pipeline);

        const auto layout = pipeline_.getRelated<UsesLayout, PipelineLayout>();

        auto scene_camera = world_->getSingleton<SceneCamera>();
        auto frustum = world_->get<CameraFrustum>(scene_camera->camera);

        DirectX::XMVECTOR planes[6];
        frustum->frustum.GetPlanes(
            &planes[0], &planes[1], &planes[2], &planes[3], &planes[4],
            &planes[5]
        );

        std::atomic<size_t> ix = 0;
        {
            OPTICK_EVENT("Collect instances")
            auto res = world_->getResults(worldObjects_);
            {
                OPTICK_EVENT("Resize");
                if (instances.size() < res.count() * 2) {
                    instances.resize(res.count() * 2);
                    mats.resize(res.count() * 2);
                }
            }

            res.each<WorldTransform, Transforms::LocalBoundingSphere, Mesh>(
                [&](ecs::EntityHandle e,
                    const WorldTransform * wt,
                    const Transforms::LocalBoundingSphere * lbs,
                    const Mesh * mesh
                ) {
                    DirectX::BoundingSphere bs;
                    auto tx = XMLoadFloat4x4(&wt->transform);
                    lbs->boundSphere.Transform(bs, tx);

                    for (auto & plane: planes) {
                        DirectX::XMVECTOR c = DirectX::XMLoadFloat3(&bs.Center);
                        c = DirectX::XMVectorSetW(c, 1.f);

                        DirectX::XMVECTOR Dist = DirectX::XMVector4Dot(c, plane);
                        if (DirectX::XMVectorGetX(Dist) > bs.Radius) {
                            return;
                        }
                    }
                    for (auto & sm: mesh->subMeshes) {
                        auto rdc = world_->get<RenderDetailCache>(sm);
                        if (!rdc || !rdc->opaquePipeline) {
                            return;
                        }
                        {
                            size_t ix2 = ix++;
                            //size_t ix = mats.size();
                            mats[ix2] = wt->transform;

                            instances[ix2] = {
                                rdc->opaquePipeline, rdc->bundle, rdc->vertexOffset,
                                rdc->indexOffset,
                                rdc->indexCount, rdc->material, static_cast<uint32_t>(ix2)
                            };
                        }
                    }
                }
            );
        }
        {
            OPTICK_EVENT("Sort Meshes")
            std::sort(
                instances.begin(),
                instances.begin() + ix,
                [](const auto & a, const auto & b) {
                    if (a.pipeline < b.pipeline) {
                        return true;
                    }
                    if (a.pipeline > b.pipeline) {
                        return false;
                    }
                    return (a.bundle < b.bundle);
                }
            );
        }
        IndirectDrawSet ids;
        {
            OPTICK_EVENT("Build Draw Commands")
            ecs::entity_t prevPL = 0;
            ecs::entity_t prevBundle = 0;
            //uint32_t prevMix = RX_INVALID_ID;
            //uint32_t prevVertexOffset = std::numeric_limits<uint32_t>::max();

            uint32_t headerIndex = 0;
            uint32_t commandIndex = 0;

            for (size_t i = 0; i < ix; i++) {
                auto & instance = instances[i];

                if (prevPL != instance.pipeline || instance.bundle != prevBundle) {

                    headerIndex = static_cast<uint32_t>(ids.headers.size());
                    ids.headers
                       .push_back(
                           IndirectDrawCommandHeader{
                               instance.pipeline,
                               instance.bundle,
                               static_cast<uint32_t>(ids.commands.size()),
                               0
                           }
                       );

                    prevPL = instance.pipeline;
                    prevBundle = instance.bundle;
                    //prevVertexOffset = instance.vertexOffset;
                }

                commandIndex = static_cast<uint32_t>(ids.commands.size());
                ids.commands.push_back(
                    {
                        instance.indexCount,
                        instance.vertexOffset,
                        instance.indexOffset, 0, static_cast<uint32_t>(ids.instances.size())
                    }
                );
                ids.headers[headerIndex].commandCount++;

                auto mm = world_->get<Material>(instance.material);

                ids.instances.push_back({mats[instance.matrixIndex], mm->sequence, 0, 0, 0});
                ids.commands[commandIndex].instanceCount++;
            }
        }
        if (ids.instances.empty()) {
            return;
        }

        auto sib = world_->getSingletonUpdate<DynamicInstanceBuffers>();
        sib->ix = (sib->ix + 1) % sib->count;
        if (sib->sizes[sib->ix] < ids.instances.size()) {
            auto n = ids.instances.size() * 2;
            auto b = engine_->createStorageBuffer(n * sizeof(IndirectDrawInstance));

            sib->buffers[sib->ix] = b;
            b->map();
            sib->sizes[sib->ix] = static_cast<uint32_t>(n);
        }

        sib->buffers[sib->ix]->update(
            ids.instances.data(),
            ids.instances.size() * sizeof(IndirectDrawInstance));

        auto cmds = world_->getSingleton<CurrentMainDescriptorSet>();
        auto ds0 = world_->get<DescriptorSet>(cmds->descriptorSet);
        auto windowDetails = world_->getSingleton<WindowDetails>();
        auto buf = RxCore::threadResources.getCommandBuffer();

        bool flipY = true;

        buf->begin(pipeline->renderPass, pipeline->subPass);
        {
            buf->useLayout(layout->layout);
            OPTICK_GPU_CONTEXT(buf->Handle())
            OPTICK_GPU_EVENT("Draw DynamicMesh")
            buf->BindDescriptorSet(0, ds0->ds);

            buf->setScissor(
                {
                    {0,                    0},
                    {windowDetails->width, windowDetails->height}
                }
            );
            buf->setViewport(
                .0f, flipY ? static_cast<float>(windowDetails->height) : 0.0f,
                static_cast<float>(windowDetails->width),
                flipY
                ? -static_cast<float>(windowDetails->height)
                : static_cast<float>(windowDetails->height), 0.0f,
                1.0f
            );

            //buf->BindDescriptorSet(2, sib->descriptorSets[sib->ix]);
            auto da = sib->buffers[sib->ix]->getDeviceAddress();
            buf->pushConstant(vk::ShaderStageFlagBits::eVertex, 8, sizeof(da), &da);
            renderIndirectDraws(ids, buf);
            //buf->BindPipeline(pipeline->pipeline->Handle());
        }
        buf->end();

        world_->getStream<Render::OpaqueRenderCommand>()
              ->add<Render::OpaqueRenderCommand>({buf});
    }

    void DynamicMeshModule::renderIndirectDraws(
        IndirectDrawSet ids,
        const std::shared_ptr<RxCore::SecondaryCommandBuffer> & buf) const
    {
        OPTICK_EVENT()
        ecs::entity_t current_pipeline{};
        ecs::entity_t prevBundle = 0;

        for (auto & h: ids.headers) {
            OPTICK_EVENT("IDS Header")
            if (h.commandCount == 0) {
                continue;
            }
            {
                OPTICK_EVENT("Set Pipeline and buffers")
                if (h.pipelineId != current_pipeline) {

                    auto pl = world_->get<GraphicsPipeline>(h.pipelineId);
                    buf->bindPipeline(pl->pipeline->Handle());
                    current_pipeline = h.pipelineId;
                }
                if (h.bundle != prevBundle) {

                    OPTICK_EVENT("Bind Bundle")
                    auto bund = world_->get<MeshBundle>(h.bundle);
                    {
                        //                        if (bund->useDescriptor) {
                        OPTICK_EVENT("Bind Ds")
                        //buf->BindDescriptorSet(1, bund->descriptorSet);
                        buf->pushConstant(
                            vk::ShaderStageFlagBits::eVertex, 0,
                            sizeof(bund->address), &bund->address
                        );
                        //                        } else {
                        //                          OPTICK_EVENT("Bind VB")
                        //                        buf->bindVertexBuffer(bund->vertexBuffer);
                        //                  }
                    }
                    {
                        OPTICK_EVENT("Bind IB")
                        buf->bindIndexBuffer(bund->indexBuffer);
                    }
                    // bind bundle descriptorSet
                    prevBundle = h.bundle;
                }
            }
            {
                OPTICK_EVENT("Draw Indexed")
                for (uint32_t i = 0; i < h.commandCount; i++) {
                    auto & c = ids.commands[i + h.commandStart];
                    buf->DrawIndexed(
                        c.indexCount, c.instanceCount, c.indexOffset, c.vertexOffset,
                        c.instanceOffset
                    );
                }
            }
        }
    }
}
