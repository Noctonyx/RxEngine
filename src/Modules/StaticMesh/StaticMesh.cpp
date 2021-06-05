#include "StaticMesh.h"

#include "EngineMain.hpp"
#include "imgui.h"
#include "Loader.h"
#include "Modules/Render.h"
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
        worldObjects_ = world_->createQuery()
                .with<WorldObject, WorldTransform, HasVisiblePrototype>()
                .withJob()
                //.withRelation<HasVisiblePrototype, VisiblePrototype>()
                .withInheritance(true).id;

        world_->addSingleton<StaticInstanceBuffers>();

        auto sib = world_->getSingletonUpdate<StaticInstanceBuffers>();

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
       // auto pl = world_->lookup("layout/general").get<PipelineLayout>();

        for (uint32_t i = 0; i < sib->count; i++) {

         //   sib->descriptorSets[i] = RxCore::threadResources.getDescriptorSet(
          //      pool_template, pl->dsls[2]);
        }

        world_->createSystem("StaticMesh:Render")
              .inGroup("Pipeline:Render")
              .withWrite<Render::OpaqueRenderCommand>()
              .withRead<CurrentMainDescriptorSet>()
              .withRead<DescriptorSet>()
              .withRead<PipelineLayout>()
              .withRead<VisiblePrototype>()
              .execute([this](ecs::World *)
              {
                  OPTICK_EVENT("StaticMesh:Render")
                  createOpaqueRenderCommands();
              });


        //        world_->createSystem("StaticMesh:Render")
        //      .inGroup("Pipeline:PreRender")
        //     .withQuery<Sta>()

        
        pipeline_ = world_->lookup("pipeline/staticmesh_opaque");
    }

    void StaticMeshModule::shutdown() { }

    ecs::entity_t createStaticMeshBundle(ecs::World * world)
    {
        auto mbe = world->newEntity();

        auto mb = mbe.addAndUpdate<MeshBundle>();

        mb->vertexCount = 0;
        mb->indexCount = 0;
        mb->vertexSize = sizeof(StaticMeshVertex);
        //mb->useDescriptor = true;
        mb->maxVertexCount = (256 * 1024 * 1024 / mb->vertexSize);
        mb->maxIndexCount = mb->maxVertexCount;

        mb->vertexBuffer = RxCore::iVulkan()->createBuffer(
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer |
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress,
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
        //vk::BufferDeviceAddressInfo bdai{};
        //bdai.setBuffer(mb->vertexBuffer->handle());

        mb->address = mb->vertexBuffer->getDeviceAddress();//  RxCore::iVulkan()->VkDevice().getBufferAddress(bdai);

        world->getSingletonUpdate<StaticMeshActiveBundle>()->currentBundle = mbe.id;
        return mbe.id;
    }

    ecs::entity_t getActiveMeshBundle(ecs::World * world)
    {
        auto smab = world->getSingleton<StaticMeshActiveBundle>();
        if(!smab) {
            world->addSingleton<StaticMeshActiveBundle>();
            smab = world->getSingleton<StaticMeshActiveBundle>();
        }

        if (world->isAlive(smab->currentBundle)) {
            return smab->currentBundle;
        }

        return createStaticMeshBundle(world);
    }

    void copyToBuffers(const std::vector<StaticMeshVertex> & meshVertices,
                       const std::vector<uint32_t> & meshIndices,
                       MeshBundle * smb)
    {
        size_t v_size = meshVertices.size() * sizeof(StaticMeshVertex);
        size_t i_size = meshIndices.size() * sizeof(uint32_t);

        auto cb = RxCore::iVulkan()->transferCommandPool_->createTransferCommandBuffer();
        cb->begin();

        const auto b1 = RxCore::iVulkan()->createStagingBuffer(v_size, meshVertices.data());
        const auto b2 = RxCore::iVulkan()->createStagingBuffer(i_size, meshIndices.data());

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

        std::copy(msd.indices.begin(), msd.indices.end(), mesh_indices.begin());
        std::transform(
            msd.vertices.begin(), msd.vertices.end(), mesh_vertices.begin(),
            [](RxAssets::MeshSaveVertex & m)
            {
                return StaticMeshVertex{
                    {m.x, m.y, m.z}, 0.f, {m.nx, m.ny, m.nz}, 0.f, {m.uvx, m.uvy}, 0.f,
                    0.f
                };
            });

        auto mb = getActiveMeshBundle(world);
        {
            auto smb = world->get<MeshBundle>(mb);

            if (mesh_vertices.size() + smb->vertexCount > smb->maxVertexCount || mesh_indices.size()
                +
                smb->indexCount > smb->maxIndexCount) {
                mb = createStaticMeshBundle(world);
                world->getSingletonUpdate<StaticMeshActiveBundle>()->currentBundle = mb;
            }
        }

        auto smb = world->getUpdate<MeshBundle>(mb);


        auto static_mesh_entity = world->newEntity(meshName.c_str())
                                       .set<Mesh>({
                                           .vertexOffset = smb->vertexCount,
                                           .indexOffset = smb->indexCount,
                                           .indexCount = static_cast<uint32_t>(mesh_indices.size())
                                       })
                                       .set<InBundle>({{mb}});

        copyToBuffers(mesh_vertices, mesh_indices, smb);

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
        frustum->frustum.GetPlanes(&planes[0], &planes[1], &planes[2], &planes[3], &planes[4],
                                   &planes[5]);

        std::mutex g;
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

                    for (auto & plane : planes) {
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
                            //size_t ix = mats.size();
                            mats[ix2] = wt->transform;

                            instances[ix2] = {
                                rdc->opaquePipeline, rdc->bundle, rdc->vertexOffset,
                                rdc->indexOffset,
                                rdc->indexCount, rdc->material, static_cast<uint32_t>(ix2)
                            };
                        }
                    }
                });
        }
        {
            OPTICK_EVENT("Sort Instances")
            std::sort(
                instances.begin(),
                instances.begin() + ix,
                [](const auto & a, const auto & b)
                {
                    if (a.pipeline < b.pipeline) {
                        return true;
                    }
                    if (a.pipeline > b.pipeline) {
                        return false;
                    }
                    if (a.bundle < b.bundle) {
                        return true;
                    }
                    if (a.bundle > b.bundle) {
                        return false;
                    }
                    return a.vertexOffset < b.vertexOffset;
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

                if (prevPL != instance.pipeline || instance.bundle != prevBundle) {

                    headerIndex = static_cast<uint32_t>(ids.headers.size());
                    ids.headers
                       .push_back(IndirectDrawCommandHeader{
                           instance.pipeline,
                           instance.bundle,
                           static_cast<uint32_t>(ids.commands.size()),
                           0
                       });

                    prevPL = instance.pipeline;
                    prevBundle = instance.bundle;
                    //prevVertexOffset = instance.vertexOffset;
                }

                if (instance.vertexOffset != prevVertexOffset) {
                    commandIndex = static_cast<uint32_t>(ids.commands.size());
                    //auto mesh = bundle->getEntry(mix);
                    ids.commands.push_back({
                            instance.indexCount,
                            instance.vertexOffset,
                            instance.indexOffset, 0, static_cast<uint32_t>(ids.instances.size())
                        }
                    );
                    ids.headers[headerIndex].commandCount++;
                    prevVertexOffset = instance.vertexOffset;
                }
                //auto& e = (*entities)[eix];

                auto mm = world_->get<Material>(instance.material);


                ids.instances.push_back({mats[instance.matrixIndex], mm->sequence, 0, 0, 0});
                ids.commands[commandIndex].instanceCount++;

                if (ids.instances.size() > 19990) {
                    //spdlog::info("too many instances");
                    //break;
                }
            }
        }

        if (ids.instances.empty()) {
            return;
        }

        auto sib = world_->getSingletonUpdate<StaticInstanceBuffers>();
        sib->ix = (sib->ix + 1) % sib->count;
        if (sib->sizes[sib->ix] < ids.instances.size()) {
            auto n = ids.instances.size() * 2;
            auto b = engine_->createStorageBuffer(n * sizeof(IndirectDrawInstance));

            sib->buffers[sib->ix] = b;
            b->map();
            sib->sizes[sib->ix] = static_cast<uint32_t>(n);
            //sib->descriptorSets[sib->ix]->
//                updateDescriptor(0, vk::DescriptorType::eStorageBuffer, b);
        }


        sib->buffers[sib->ix]->update(ids.instances.data(),
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
            OPTICK_GPU_EVENT("Draw StaticMesh")
            buf->BindDescriptorSet(0, ds0->ds);

            buf->setScissor(
                {
                    {0, 0},
                    {windowDetails->width, windowDetails->height}
                });
            buf->setViewport(
                .0f, flipY ? static_cast<float>(windowDetails->height) : 0.0f,
                static_cast<float>(windowDetails->width),
                flipY
                    ? -static_cast<float>(windowDetails->height)
                    : static_cast<float>(windowDetails->height), 0.0f,
                1.0f);

            //buf->BindDescriptorSet(2, sib->descriptorSets[sib->ix]);
            vk::DeviceAddress da = sib->buffers[sib->ix]->getDeviceAddress();
            buf->pushConstant(vk::ShaderStageFlagBits::eVertex, 8, sizeof(vk::DeviceAddress), &da);
            renderIndirectDraws(ids, buf);
            //buf->BindPipeline(pipeline->pipeline->Handle());
        }
        buf->end();

        world_->getStream<Render::OpaqueRenderCommand>()
              ->add<Render::OpaqueRenderCommand>({buf});
    }

    void StaticMeshModule::renderIndirectDraws(
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
                            buf->pushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(vk::DeviceAddress), &bund->address);
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
                        c.instanceOffset);
                }
            }
        }
    }
}
