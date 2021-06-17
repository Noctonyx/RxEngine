////////////////////////////////////////////////////////////////////////////////
// MIT License
//
// Copyright (c) 2021-2021.  Shane Hyde (shane@noctonyx.com)
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

#include <Vulkan/Buffer.hpp>
#include "StaticMesh.h"

#include "EngineMain.hpp"
#include "imgui.h"
#include "Loader.h"
#include "Modules/Render.h"
#include "Modules/Lighting/Lighting.h"
#include "Modules/Materials/Materials.h"
#include "Modules/Prototypes/Prototypes.h"
#include "Modules/RTSCamera/RTSCamera.h"
#include "Modules/WorldObject/WorldObject.h"
#include "sol/state.hpp"
#include "sol/table.hpp"
#include "Vulkan/ThreadResources.h"
#include "Modules/SceneCamera/SceneCamera.h"
#include "Modules/Mesh/Mesh.h"

namespace RxEngine
{
    struct SceneCamera;

    void StaticMeshModule::startup()
    {
        //world_->addSingleton<StaticMeshActiveBundle>();
        worldObjects_ = world_->createQuery<WorldObject, WorldTransform, HasVisiblePrototype>()
                              .withJob()
                              //.withRelation<HasVisiblePrototype, VisiblePrototype>()
                              .withInheritance(true).id;

        instanceBuffers.count = 5;
        instanceBuffers.sizes.resize(5);
        instanceBuffers.buffers.resize(5);
        //sib->descriptorSets.resize(5);
#if 0
        const RxCore::DescriptorPoolTemplate pool_template(
            {
                {
                    VkDescriptorType::eStorageBuffer,
                    10
                }
            }, 10);
#endif
        world_->createSystem("StaticMesh:Render")
              .inGroup("Pipeline:Render")
              .withStreamWrite<Render::OpaqueRenderCommand>()
              .withRead<CurrentMainDescriptorSet>()
              .withRead<DescriptorSet>()
              .withRead<PipelineLayout>()
              .withRead<VisiblePrototype>()
              .withJob()
              .execute(
                  [this](ecs::World *)
                  {
                      OPTICK_EVENT("StaticMesh:Render")
                      createOpaqueRenderCommands();
                  }
              );

        world_->createSystem("StaticMesh:ShadowRender")
              .inGroup("Pipeline:Render")
              .withStreamWrite<Render::ShadowRenderCommand>()
              .withRead<CurrentMainDescriptorSet>()
              .withRead<DescriptorSet>()
              .withRead<PipelineLayout>()
              .withRead<VisiblePrototype>()
              .withRead<ShadowCascadeData>()
              .execute(
                  [](ecs::World *)
                  {
                      OPTICK_EVENT("StaticMesh:ShadowRender")
                      //createOpaqueRenderCommands();
                  }
              );

        //pipeline_ = world_->lookup("pipeline/staticmesh_opaque");
    }

    void StaticMeshModule::shutdown() {}

    ecs::entity_t createStaticMeshBundle(RxCore::Device * device, ecs::World * world)
    {
        auto mbe = world->newEntity();

        auto mb = mbe.addAndUpdate<MeshBundle>();

        mb->vertexCount = 0;
        mb->indexCount = 0;
        mb->vertexSize = sizeof(StaticMeshVertex);
        //mb->useDescriptor = true;
        mb->maxVertexCount = (256 * 1024 * 1024 / mb->vertexSize);
        mb->maxIndexCount = mb->maxVertexCount;

        mb->vertexBuffer = device->createBuffer(
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY, mb->maxVertexCount * mb->vertexSize
        );

        mb->indexBuffer = device->createIndexBuffer(
            VMA_MEMORY_USAGE_GPU_ONLY, static_cast<uint32_t>(mb->maxVertexCount * sizeof(uint32_t)),
            false
        );
#if 0
        const RxCore::DescriptorPoolTemplate pool_template(
            {
                {
                    VkDescriptorType::eStorageBuffer,
                    10
                }
            }, 10);
#endif
        //auto pl = world->lookup("layout/general").get<PipelineLayout>();
#if 0
        mb->descriptorSet = RxCore::threadResources.getDescriptorSet(pool_template, pl->dsls[1]);
        mb->descriptorSet->
            updateDescriptor(0, VkDescriptorType::eStorageBuffer, mb->vertexBuffer);
#endif
        //VkBufferDeviceAddressInfo bdai{};
        //bdai.setBuffer(mb->vertexBuffer->handle());

        mb->address = mb->vertexBuffer->getDeviceAddress();
        //  RxCore::iVulkan()->VkDevice().getBufferAddress(bdai);

        world->getSingletonUpdate<StaticMeshActiveBundle>()->currentBundle = mbe.id;
        return mbe.id;
    }

    ecs::entity_t getActiveMeshBundle(RxCore::Device * device, ecs::World * world)
    {
        auto smab = world->getSingleton<StaticMeshActiveBundle>();
        if (!smab) {
            world->addSingleton<StaticMeshActiveBundle>();
            smab = world->getSingleton<StaticMeshActiveBundle>();
        }

        if (world->isAlive(smab->currentBundle)) {
            return smab->currentBundle;
        }

        return createStaticMeshBundle(device, world);
    }

    void copyToBuffers(RxCore::Device * device,
                       const std::vector<StaticMeshVertex> & meshVertices,
                       const std::vector<uint32_t> & meshIndices,
                       MeshBundle * smb)
    {
        size_t v_size = meshVertices.size() * sizeof(StaticMeshVertex);
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

    void loadMesh(ecs::World * world,
                  RxCore::Device * device,
                  std::string meshName,
                  sol::table details)
    {
        //auto mbe = getActiveMeshBundle(world);

        std::string meshFile = details.get_or("mesh", std::string{""});
        auto vertices = details.get<uint32_t>("vertices");
        auto indices = details.get<uint32_t>("indices");

        std::vector<StaticMeshVertex> mesh_vertices(vertices);
        std::vector<uint32_t> mesh_indices(indices);

        RxAssets::MeshSaveData msd;

        RxAssets::Loader::loadMesh(msd, meshFile);
        DirectX::BoundingBox bb;
        DirectX::XMFLOAT3 p1f(msd.minpx, msd.minpy, msd.minpz);
        DirectX::XMFLOAT3 p2f(msd.maxpx, msd.maxpy, msd.maxpz);

        DirectX::XMVECTOR p1, p2;

        p1 = DirectX::XMLoadFloat3(&p1f);
        p2 = DirectX::XMLoadFloat3(&p2f);

        DirectX::BoundingBox::CreateFromPoints(bb, p1, p2);

        DirectX::BoundingSphere bs;
        DirectX::BoundingSphere::CreateFromBoundingBox(bs, bb);

        std::copy(msd.indices.begin(), msd.indices.end(), mesh_indices.begin());
        std::transform(
            msd.vertices.begin(), msd.vertices.end(), mesh_vertices.begin(),
            [](RxAssets::MeshSaveVertex & m)
            {
                return StaticMeshVertex{
                    {m.x, m.y, m.z}, 0.f, {m.nx, m.ny, m.nz}, 0.f, {m.uvx, m.uvy}, 0.f,
                    0.f
                };
            }
        );

        auto mb = getActiveMeshBundle(device, world);
        {
            auto smb = world->get<MeshBundle>(mb);

            if (mesh_vertices.size() + smb->vertexCount > smb->maxVertexCount || mesh_indices.size()
                +
                smb->indexCount > smb->maxIndexCount) {
                mb = createStaticMeshBundle(device, world);
                world->getSingletonUpdate<StaticMeshActiveBundle>()->currentBundle = mb;
            }
        }

        auto smb = world->getUpdate<MeshBundle>(mb);

        auto static_mesh_entity = world->newEntity(meshName.c_str())
                                       .set<Mesh>(
                                           {
                                               .vertexOffset = smb->vertexCount,
                                               .indexOffset = smb->indexCount,
                                               .indexCount = static_cast<uint32_t>(mesh_indices.
                                                   size()),
                                               .boundSphere = bs
                                           }
                                       )
                                       .set<InBundle>({{mb}});

        copyToBuffers(device, mesh_vertices, mesh_indices, smb);

        smb->entries.push_back(static_mesh_entity.id);

        //auto sm = mesh_prim_entity.addAndUpdate<StaticMesh>();

        sol::table smtab = details.get<sol::table>("submeshes");
        sol::table mtab = details.get<sol::table>("materials");

        std::vector<ecs::entity_t> mEntities;

        for (auto & [k, v]: mtab) {
            std::string mn = v.as<std::string>();

            auto mat_entity = world->lookup(mn.c_str());
            if (mat_entity.isAlive() && mat_entity.has<Material>()) {
                mEntities.push_back(mat_entity.id);
            }
        }

        auto smu = static_mesh_entity.getUpdate<Mesh>();

        uint32_t ix = 0;
        for (auto & [k, v]: smtab) {
            sol::table subMeshValue = v;

            uint32_t first_index = subMeshValue.get<uint32_t>("first_index");
            uint32_t index_count = subMeshValue.get<uint32_t>("index_count");
            const uint32_t material = subMeshValue.get<uint32_t>("material");

            //sm->subMeshes.push_back(
            smu->subMeshes.push_back(
                world->newEntity()
                     //.set<ecs::InstanceOf>({{me.id}})
                     .set<SubMesh>({first_index, index_count, ix++})
                     .set<SubMeshOf>({{static_mesh_entity.id}})
                     .set<UsesMaterial>({{mEntities[material]}}).id
            );
        }
    }

    void loadMeshes(ecs::World * world, RxCore::Device * device, sol::table & meshes)
    {
        for (auto & [key, value]: meshes) {
            const std::string name = key.as<std::string>();
            const sol::table details = value;
            loadMesh(world, device, name, details);
        }
    }

    void StaticMeshModule::loadData(sol::table data)
    {
        sol::optional<sol::table> meshes = data["mesh"];

        if (meshes.has_value()) {
            loadMeshes(world_, engine_->getDevice(), meshes.value());
        }
    }

    void StaticMeshModule::createOpaqueRenderCommands()
    {
        OPTICK_CATEGORY("Render Static", ::Optick::Category::Rendering)

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

            res.each<WorldTransform, HasVisiblePrototype>(
                [&](ecs::EntityHandle e,
                    const WorldTransform * wt,
                    const HasVisiblePrototype * vpp)
                {
                    auto vp = world_->get<VisiblePrototype>(vpp->entity);
                    //OPTICK_EVENT("Process Entity")
                    if (!vp) {
                        return;
                    }
                    DirectX::BoundingSphere bs;
                    auto tx = XMLoadFloat4x4(&wt->transform);
                    vp->boundingSphere.Transform(bs, tx);

                    for (auto & plane: planes) {
                        DirectX::XMVECTOR c = DirectX::XMLoadFloat3(&bs.Center);
                        c = DirectX::XMVectorSetW(c, 1.f);

                        DirectX::XMVECTOR Dist = DirectX::XMVector4Dot(c, plane);
                        if (DirectX::XMVectorGetX(Dist) > bs.Radius) {
                            return;
                        }
                    }
                    for (auto & sm: vp->subMeshEntities) {
                        auto rdc = world_->get<RenderDetailCache>(sm);
                        if (!rdc || !rdc->opaquePipeline) {
                            return;
                        }
                        {
                            size_t ix2 = ix++;
                            mats[ix2] = wt->transform;

                            instances[ix2] = {
                                rdc, rdc->opaquePipeline,
                                static_cast<uint32_t>(ix2)
                            };
                        }
                    }
                }
            );
        }
        {
            OPTICK_EVENT("Sort Instances")
            std::sort(
                instances.begin(),
                instances.begin() + ix,
                [](const auto & a, const auto & b)
                {
                    auto & [ar, apipeline, am] = a;
                    auto & [br, bpipeline, bm] = b;
                    if (apipeline < bpipeline) {
                        return true;
                    }
                    if (apipeline > bpipeline) {
                        return false;
                    }
                    if (ar->bundle < br->bundle) {
                        return true;
                    }
                    if (ar->bundle > br->bundle) {
                        return false;
                    }
                    return ar->vertexOffset < br->vertexOffset;
                }
            );
        }
        IndirectDrawSet ids;

        {
            OPTICK_EVENT("Build Draw Commands")
            ecs::entity_t prevPL = 0;
            ecs::entity_t prevBundle = 0;
            //uint32_t prevMix = RX_INVALID_ID;
            uint32_t prevVertexOffset = std::numeric_limits<uint32_t>::max();

            uint32_t headerIndex = 0;
            uint32_t commandIndex = 0;

            for (size_t i = 0; i < ix; i++) {
                auto & instance = instances[i];
                auto & [rdc, rpipeline, m] = instance;

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

                if (rdc->vertexOffset != prevVertexOffset) {
                    commandIndex = static_cast<uint32_t>(ids.commands.size());
                    ids.commands.push_back(
                        {
                            rdc->indexCount,
                            rdc->vertexOffset,
                            rdc->indexOffset, 0, static_cast<uint32_t>(ids.instances.size())
                        }
                    );
                    ids.headers[headerIndex].commandCount++;
                    prevVertexOffset = rdc->vertexOffset;
                }

                auto mm = world_->get<Material>(rdc->material);

                ids.instances.push_back({mats[m], mm->sequence, 0, 0, 0});
                ids.commands[commandIndex].instanceCount++;
            }
        }

        if (ids.instances.empty()) {
            return;
        }

        createInstanceBuffer(ids);
        MeshModule::drawInstances(instanceBuffers.buffers[instanceBuffers.ix], world_, pipeline,
                                  layout, ids);
    }

    void StaticMeshModule::createInstanceBuffer(IndirectDrawSet & ids)
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
