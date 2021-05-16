#include "StaticMesh.h"

#include "EngineMain.hpp"
#include "imgui.h"
#include "Loader.h"
#include "Modules/Render.h"
#include "Modules/Materials/Materials.h"
#include "Modules/Prototypes/Prototypes.h"
#include "Modules/RTSCamera/RTSCamera.h"
#include "Modules/Transforms/Transforms.h"
#include "Modules/WorldObject/WorldObject.h"
#include "sol/state.hpp"
#include "sol/table.hpp"
#include "Vulkan/ThreadResources.h"
#include "Modules/SceneCamera/SceneCamera.h"

using namespace DirectX;

namespace RxEngine
{
    struct SceneCamera;

    void staticMeshBundleGui(ecs::World *, void * ptr)
    {
        auto mesh_bundle = static_cast<StaticMeshBundle *>(ptr);

        if (mesh_bundle) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Vertex Size");
            ImGui::TableNextColumn();
            ImGui::Text("%d", mesh_bundle->vertexSize);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Vertex Count");
            ImGui::TableNextColumn();
            ImGui::Text("%d/%d", mesh_bundle->vertexCount, mesh_bundle->maxVertexCount);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Index Count");
            ImGui::TableNextColumn();
            ImGui::Text("%d/%d", mesh_bundle->indexCount, mesh_bundle->maxIndexCount);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Mesh Count");
            ImGui::TableNextColumn();
            ImGui::Text("%lld", mesh_bundle->entries.size());
        }
    }

    void meshPrimitiveGui(ecs::World *, void * ptr)
    {
        auto mesh = static_cast<StaticMesh *>(ptr);

        if (mesh) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Vertex Offset");
            ImGui::TableNextColumn();
            ImGui::Text("%d", mesh->vertexOffset);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Index Offset");
            ImGui::TableNextColumn();
            ImGui::Text("%d", mesh->indexOffset);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Index Count");
            ImGui::TableNextColumn();
            ImGui::Text("%d", mesh->indexCount);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("SphereBounds");
            ImGui::TableNextColumn();
            ImGui::Text("(%.2f,%.2f,%.2f) %.2f", mesh->boundSphere.Center.x,
                        mesh->boundSphere.Center.y, mesh->boundSphere.Center.z,
                        mesh->boundSphere.Radius);
        }
    }

    void subMeshGui(ecs::World *, void * ptr)
    {
        auto sub_mesh = static_cast<SubMesh *>(ptr);

        if (sub_mesh) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Index Offset");
            ImGui::TableNextColumn();
            ImGui::Text("%d", sub_mesh->indexOffset);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Index Count");
            ImGui::TableNextColumn();
            ImGui::Text("%d", sub_mesh->indexCount);
        }
    }

    void StaticMeshModule::registerModule()
    {
        world_->addSingleton<StaticMeshActiveBundle>();
        world_->set<ComponentGui>(world_->getComponentId<StaticMeshBundle>(),
                                  ComponentGui{.editor = staticMeshBundleGui});
        world_->set<ComponentGui>(world_->getComponentId<StaticMesh>(),
                                  ComponentGui{.editor = meshPrimitiveGui});
        world_->set<ComponentGui>(world_->getComponentId<SubMesh>(),
                                  ComponentGui{.editor = subMeshGui});
    }

    void cacheMeshRenderDetails(ecs::EntityHandle subMeshEntity)
    {
        RenderDetailCache rdc{};

        auto static_mesh_entity = subMeshEntity.getRelatedEntity<SubMeshOf>();
        if (!static_mesh_entity) {
            return;
        }

        auto static_mesh = static_mesh_entity.get<StaticMesh>();

        rdc.vertexOffset = static_mesh->vertexOffset;
        rdc.indexCount = static_mesh->indexCount;
        rdc.indexOffset = static_mesh->indexOffset;
        rdc.boundSphere = static_mesh->boundSphere;

        const auto bundle_entity = static_mesh_entity.getRelatedEntity<InBundle>();
        if (!bundle_entity) {
            return;
        }
        rdc.bundle = bundle_entity;

        auto material_entity = subMeshEntity.getRelatedEntity<UsesMaterial>();
        if (!material_entity) {
            return;
        }

        rdc.material = material_entity;

        rdc.opaquePipeline = material_entity.getRelatedEntity<HasOpaquePipeline>();
        rdc.shadowPipeline = material_entity.getRelatedEntity<HasShadowPipeline>();
        rdc.transparentPipeline = material_entity.getRelatedEntity<HasTransparentPipeline>();

        subMeshEntity.setDeferred(rdc);
    }

    void StaticMeshModule::startup()
    {
        world_->addSingleton<StaticInstanceBuffers>();

        auto sib = world_->getSingletonUpdate<StaticInstanceBuffers>();

        sib->count = 5;
        sib->sizes.resize(5);
        sib->buffers.resize(5);
        sib->descriptorSets.resize(5);

        RxCore::DescriptorPoolTemplate poolTemplate({{vk::DescriptorType::eStorageBuffer, 10}}, 10);
        auto pl = world_->lookup("layout/general").get<PipelineLayout>();

        for (uint32_t i = 0; i < sib->count; i++) {

            sib->descriptorSets[i] = RxCore::threadResources.getDescriptorSet(
                poolTemplate, pl->dsls[2]);
        }

        world_->createSystem("StaticMesh:Render")
              .inGroup("Pipeline:PreRender")
              .withWrite<Render::OpaqueRenderCommand>()
              .withRead<CurrentMainDescriptorSet>()
              .withRead<DescriptorSet>()
              .withRead<PipelineLayout>()
              .execute([this](ecs::World *)
              {
                  OPTICK_EVENT("StaticMesh:Render")
                  createOpaqueRenderCommands();
              });

        world_->createSystem("StaticMesh:PrepareMeshes")
              .inGroup("Pipeline:PreFrame")
              .withQuery<SubMesh>()
              .without<RenderDetailCache>()
              .each(cacheMeshRenderDetails);

        //        world_->createSystem("StaticMesh:Render")
        //      .inGroup("Pipeline:PreRender")
        //     .withQuery<Sta>()

        worldObjects = world_->createQuery()
                             .with<WorldObject, WorldTransform>()
                             .withRelation<HasVisiblePrototype, VisiblePrototype>()
                             .withInheritance(true).id;

        pipeline_ = world_->lookup("pipeline/staticmesh_opaque");
    }

    void StaticMeshModule::shutdown() { }

    ecs::entity_t createStaticMeshBundle(ecs::World * world)
    {
        auto mbe = world->newEntity();

        auto mb = mbe.addAndUpdate<StaticMeshBundle>();

        mb->vertexCount = 0;
        mb->indexCount = 0;
        mb->vertexSize = sizeof(StaticMeshVertex);
        mb->useDescriptor = true;
        mb->maxVertexCount = (256 * 1024 * 1024 / mb->vertexSize);
        mb->maxIndexCount = mb->maxVertexCount;

        mb->vertexBuffer = RxCore::iVulkan()->createBuffer(
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer |
            vk::BufferUsageFlagBits::eTransferDst,
            VMA_MEMORY_USAGE_GPU_ONLY, mb->maxVertexCount * mb->vertexSize);

        mb->indexBuffer = RxCore::iVulkan()->createIndexBuffer(
            VMA_MEMORY_USAGE_GPU_ONLY, static_cast<uint32_t>(mb->maxVertexCount * sizeof(uint32_t)),
            false);

        RxCore::DescriptorPoolTemplate poolTemplate({{vk::DescriptorType::eStorageBuffer, 10}}, 10);

        auto pl = world->lookup("layout/general").get<PipelineLayout>();
        mb->descriptorSet = RxCore::threadResources.getDescriptorSet(poolTemplate, pl->dsls[1]);
        mb->descriptorSet->
            updateDescriptor(0, vk::DescriptorType::eStorageBuffer, mb->vertexBuffer);

        world->getSingletonUpdate<StaticMeshActiveBundle>()->currentBundle = mbe.id;
        return mbe.id;
    }

    ecs::entity_t getActiveMeshBundle(ecs::World * world)
    {
        auto smab = world->getSingleton<StaticMeshActiveBundle>();
        if (world->isAlive(smab->currentBundle)) {
            return smab->currentBundle;
        }

        return createStaticMeshBundle(world);
    }

    void copyToBuffers(const std::vector<StaticMeshVertex> & meshVertices,
                       const std::vector<uint32_t> & meshIndices,
                       StaticMeshBundle * smb)
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

        std::vector<StaticMeshVertex> meshVertices(vertices);
        std::vector<uint32_t> meshIndices(indices);

        RxAssets::MeshSaveData msd;

        RxAssets::Loader::loadMesh(msd, meshFile);

        std::copy(msd.indices.begin(), msd.indices.end(), meshIndices.begin());
        std::transform(
            msd.vertices.begin(), msd.vertices.end(), meshVertices.begin(),
            [](RxAssets::MeshSaveVertex & m)
            {
                return StaticMeshVertex{
                    {m.x, m.y, m.z}, 0.f, {m.nx, m.ny, m.nz}, 0.f, {m.uvx, m.uvy}, 0.f,
                    0.f
                };
            });

        auto mb = getActiveMeshBundle(world);
        {
            auto smb = world->get<StaticMeshBundle>(mb);

            if (meshVertices.size() + smb->vertexCount > smb->maxVertexCount || meshIndices.size() +
                smb->indexCount > smb->maxIndexCount) {
                mb = createStaticMeshBundle(world);
                world->getSingletonUpdate<StaticMeshActiveBundle>()->currentBundle = mb;
            }
        }

        auto smb = world->getUpdate<StaticMeshBundle>(mb);


        auto static_mesh_entity = world->newEntity(meshName.c_str())
                                       .set<StaticMesh>({
                                           .vertexOffset = smb->vertexCount,
                                           .indexOffset = smb->indexCount,
                                           .indexCount = static_cast<uint32_t>(meshIndices.size())
                                       })
                                       .set<InBundle>({{mb}});

        copyToBuffers(meshVertices, meshIndices, smb);

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

        auto smu = static_mesh_entity.getUpdate<StaticMesh>();

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

    void StaticMeshModule::processStartupData(sol::state * lua, RxCore::Device * device)
    {
        const sol::table data = lua->get<sol::table>("data");

        sol::table meshes = data.get<sol::table>("meshes");

        loadMeshes(world_, device, meshes);
    }

    void StaticMeshModule::createOpaqueRenderCommands()
    {
        OPTICK_CATEGORY("Render Static", ::Optick::Category::Rendering);

        auto pipeline = pipeline_.get<GraphicsPipeline>();

        if (!pipeline) {
            return;
        }
        assert(pipeline);
        assert(pipeline->pipeline);

        const auto layout = pipeline_.getRelated<UsesLayout, PipelineLayout>();

        auto scene_camera = world_->getSingleton<SceneCamera>();
        auto frustum = world_->get<CameraFrustum>(scene_camera->camera);

        std::vector<ecs::entity_t> entities;

        std::vector<RenderingInstance> instances;
        std::vector<XMFLOAT4X4> mats;

        auto res = world_->getResults(worldObjects);
        res.each<WorldTransform, VisiblePrototype>(
            [&](ecs::EntityHandle e,
                const WorldTransform * wt,
                const VisiblePrototype * vp)
            {
                if (!vp) {
                    return;
                }
                BoundingSphere bs;
                auto tx = XMLoadFloat4x4(&wt->transform);
                vp->boundingSphere.Transform(bs, tx);

                if (!frustum->frustum.Intersects(bs)) {
                    return;
                }
                for (auto & sm: vp->subMeshEntities) {
                    auto rdc = world_->get<RenderDetailCache>(sm);
                    if (!rdc || !rdc->opaquePipeline) {
                        return;
                    }
                    size_t ix = mats.size();
                    mats.push_back(wt->transform);

                    instances.push_back({
                        rdc->opaquePipeline, rdc->bundle, rdc->vertexOffset, rdc->indexOffset,
                        rdc->indexCount, rdc->material, static_cast<uint32_t>(ix)
                    });
                }
            });

        std::ranges::sort(
            instances,
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

        IndirectDrawSet ids;

        {
            OPTICK_EVENT("Build Draw Commands")
            ecs::entity_t prevPL = 0;
            ecs::entity_t prevBundle = 0;
            //uint32_t prevMix = RX_INVALID_ID;
            uint32_t prevVertexOffset = std::numeric_limits<uint32_t>::max();

            uint32_t headerIndex = 0;
            uint32_t commandIndex = 0;

            for (auto & instance: instances) {
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

                ids.instances.push_back({mats[instance.matrixIndex], 0, 0, 0, 0});
                ids.commands[commandIndex].instanceCount++;

                if (ids.instances.size() > 19990) {
                    spdlog::info("too many instances");
                    break;
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
            sib->sizes[sib->ix] = n;
            sib->descriptorSets[sib->ix]->
                updateDescriptor(0, vk::DescriptorType::eStorageBuffer, b);
        }

        sib->buffers[sib->ix]->update(ids.instances.data(),
                                      ids.instances.size() * sizeof(IndirectDrawInstance));

        auto cmds = world_->getSingleton<CurrentMainDescriptorSet>();
        auto ds0 = world_->get<DescriptorSet>(cmds->descriptorSet);
        auto windowDetails = world_->getSingleton<WindowDetails>();
        auto buf = RxCore::threadResources.getCommandBuffer();

        bool flipY = false;

        buf->begin(pipeline->renderPass, pipeline->subPass);
        {
            buf->useLayout(layout->layout);
            OPTICK_GPU_CONTEXT(buf->Handle());
            OPTICK_GPU_EVENT("Draw StaticMesh");
            buf->BindDescriptorSet(0, ds0->ds);

            buf->setScissor(
                {
                    {0, 0},
                    {windowDetails->width, windowDetails->height}
                });
            buf->setViewport(
                .0f, flipY ? static_cast<float>(windowDetails->height) : 0.0f, static_cast<float>(windowDetails->width),
                flipY ? -static_cast<float>(windowDetails->height) : static_cast<float>(windowDetails->height), 0.0f,
                1.0f);

            buf->BindDescriptorSet(2, sib->descriptorSets[sib->ix]);
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
                    auto bund = world_->get<StaticMeshBundle>(h.bundle);
                    {
                        if (bund->useDescriptor) {
                            OPTICK_EVENT("Bind Ds")
                            buf->BindDescriptorSet(1, bund->descriptorSet);
                        } else {
                            OPTICK_EVENT("Bind VB")
                            buf->bindVertexBuffer(bund->vertexBuffer);
                        }
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
