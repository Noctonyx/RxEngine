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
#include <Modules/Scene/SceneModule.h>
#include "DynamicMesh.h"
#include "EngineMain.hpp"

#include "Modules/Render.h"
#include "Vulkan/ThreadResources.h"

namespace RxEngine
{
    void DynamicMeshModule::startup()
    {
        instanceBuffers.count = 5;
        instanceBuffers.sizes.resize(5);
        instanceBuffers.buffers.resize(5);

        worldObjects_ = world_->createQuery<SceneNode, WorldTransform, DynamicMesh,
                                  WorldBoundingSphere>()
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

        world_->createSystem("DynamicMesh:Cleanup")
              .inGroup("Pipeline:PostFrame")
              .withInterval(2.0f)
              .withQuery<MeshBundle, ecs::Component>()
              .each<MeshBundle>(
                  [](ecs::EntityHandle e, const MeshBundle * mb) {
                      //ecs::World * w = e.getWorld();
                      //auto filt = w->createFilter({e.id});
                      auto filt = e.getChildren();
                      if (filt.count() != 0) {
                          return;
                      }
                      e.destroyDeferred();
                  }
              );
    }

    void DynamicMeshModule::shutdown()
    {}

    ecs::entity_t createDynamicMeshBundle(RxCore::Device * device, ecs::World * world)
    {
        auto mbe = world->newEntity();

        mbe.addAndUpdate<MeshBundle>([&](MeshBundle * mb) {
            mb->vertexCount = 0;
            mb->indexCount = 0;
            mb->vertexSize = sizeof(DynamicMeshVertex);
            mb->maxVertexCount = (32 * 1024 * 1024 / mb->vertexSize);
            mb->maxIndexCount = mb->maxVertexCount * 3 / 2;

            mb->vertexBuffer = device->createBuffer(
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY, mb->maxVertexCount * mb->vertexSize
                );

            mb->indexBuffer = device->createIndexBuffer(
                VMA_MEMORY_USAGE_GPU_ONLY, static_cast<uint32_t>(mb->maxVertexCount * sizeof(uint32_t)),
                false
                );

            mb->address = mb->vertexBuffer->getDeviceAddress();
        });

        world->getSingletonUpdate<DynamicMeshActiveBundle>()->currentBundle = mbe.id;
        //world->createDynamicComponent(mbe);
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
        if (!world->has<ecs::Component>(mb)) {
            world->setAsParent(mb);
        }
        auto smb = world->get<MeshBundle>(mb);

        ecs::EntityHandle dynamic_mesh_entity;
        dynamic_mesh_entity = world->newEntity()
            .set<Mesh>(
                {
                    .vertexOffset = smb->vertexCount,
                    .indexOffset = smb->indexCount,
                    .indexCount = static_cast<uint32_t>(indices.size())
                }
                )
                .add<DynamicMesh>()
                .addParent(mb)
                .set<InBundle>({{mb}});

        world->update<MeshBundle>(mb, [&](MeshBundle * smb){
            copyToBuffers(device, vertices, indices, smb);
            smb->entries.push_back(dynamic_mesh_entity);
        });


        //auto smu = dynamic_mesh_entity.getUpdate<Mesh>();
        //std::vector<ecs::entity_t> mEntities;

        dynamic_mesh_entity.update<Mesh>([&](Mesh * smu){
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
        });


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

        std::vector<std::tuple<const RenderDetailCache *, ecs::entity_t, uint32_t>> instances;
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

            res.each<WorldTransform, WorldBoundingSphere, Mesh>(
                [&](ecs::EntityHandle e,
                    const WorldTransform * wt,
                    const WorldBoundingSphere * lbs,
                    const Mesh * mesh
                ) {
                    DirectX::BoundingSphere bs = lbs->boundSphere;

                    auto tx = XMLoadFloat4x4(&wt->transform);
//                    lbs->boundSphere.Transform(bs, tx);

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
                                rdc, rdc->opaquePipeline,
                                /*
                  rdc->opaquePipeline, rdc->bundle, rdc->vertexOffset,
                  rdc->indexOffset,
                  rdc->indexCount, rdc->material,
                                 */
                                static_cast<uint32_t>(ix2)
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
                    auto &[ar, apipeline, am] = a;
                    auto &[br, bpipeline, bm] = b;
                    if (apipeline < bpipeline) {
                        return true;
                    }
                    if (apipeline > bpipeline) {
                        return false;
                    }
                    return (ar->bundle < br->bundle);
                }
            );
        }
        IndirectDrawSet ids;
        {
            OPTICK_EVENT("Build Draw Commands")
            ecs::entity_t prevPL = 0;
            ecs::entity_t prevBundle = 0;

            uint32_t headerIndex = 0;
            uint32_t commandIndex = 0;

            for (size_t i = 0; i < ix; i++) {
                auto & instance = instances[i];
                auto &[rdc, rpipeline, m] = instance;

                if (prevPL != rpipeline || rdc->bundle != prevBundle) {

                    headerIndex = static_cast<uint32_t>(ids.headers.size());
                    ids.headers
                       .push_back(
                           IndirectDrawCommandHeader{
                               rpipeline,
                               rdc->bundle,
                               static_cast<uint32_t>(ids.commands.size()),
                               0
                           }
                       );

                    prevPL = rpipeline;
                    prevBundle = rdc->bundle;
                }

                commandIndex = static_cast<uint32_t>(ids.commands.size());
                ids.commands.push_back(
                    {
                        rdc->indexCount,
                        rdc->vertexOffset,
                        rdc->indexOffset, 0, static_cast<uint32_t>(ids.instances.size())
                    }
                );
                ids.headers[headerIndex].commandCount++;

                auto mm = world_->get<Material>(rdc->material);

                ids.instances.push_back({mats[m], mm->sequence, 0, 0, 0});
                ids.commands[commandIndex].instanceCount++;
            }
        }
        if (ids.instances.empty()) {
            return;
        }

        createInstanceBuffer(ids);
        MeshModule::drawInstances(
            instanceBuffers.buffers[instanceBuffers.ix], world_, pipeline,
            layout, ids
        );
    }

    void DynamicMeshModule::createInstanceBuffer(IndirectDrawSet & ids)
    {
        instanceBuffers.ix = (instanceBuffers.ix + 1) % instanceBuffers.count;
        if (instanceBuffers.sizes[instanceBuffers.ix] < ids.instances.size()) {
            auto n = ids.instances.size() * 2;
            auto b = engine_->createStorageBuffer(n * sizeof(IndirectDrawInstance));

            instanceBuffers.buffers[instanceBuffers.ix] = b;
            b->map();
            instanceBuffers.sizes[instanceBuffers.ix] = static_cast<uint32_t>(n);
        }

        instanceBuffers.buffers[instanceBuffers.ix]->update(
            ids.instances.data(),
            ids.instances.size() * sizeof(IndirectDrawInstance));
    }
}
