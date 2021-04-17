#include "Mesh.h"
#include <Vulkan/Device.h>
#include <RXCore.h>
#include "RxECS.h"
#include "EngineMain.hpp"
#include "imgui.h"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"

namespace RxEngine
{
    //flecs::entity Mesh::IsBundleOf;
    //flecs::entity Mesh::IsActive;
#if 0
    Mesh::MeshGroupId Mesh::createMeshGroup(flecs::world & ecs,
                                            uint32_t groupBufferSize,
                                            size_t vertexSize,
                                            bool fixedFunction)
    {
        return ecs.entity("MeshGroup").set<MeshGroup>(
            {
                groupBufferSize,
                vertexSize,
                fixedFunction
            }
        );
    }
#endif
    bool Mesh::canFit(const Mesh::MeshBundle & bundle, size_t vertexCount, size_t indexCount)
    {
        uint32_t ixCount = static_cast<uint32_t>(indexCount);
        uint32_t vxCount = static_cast<uint32_t>(vertexCount);

        if (vxCount + bundle.vertexCount_ > bundle.maxVertexCount_) {
            return false;
        }

        if (ixCount + bundle.indexCount_ > bundle.maxIndexCount_) {
            return false;
        }

        return true;
    }
#if 0
    Mesh::MeshBundleEntryId addToMeshBundle(
        const Mesh::MeshBundleId & mbId,
        Mesh::MeshBundle * bundle,
        const Mesh::MeshToBundle & mesh)
    {
        assert(canFit(bundle, mesh));

        auto meshId = mbId.world()
                          .entity()
                          .mut(mbId)
                          .add<Mesh::MeshBundleEntry>()
                          .add(flecs::ChildOf, mbId);

        auto cb = RxCore::iVulkan()->transferCommandPool_->createTransferCommandBuffer();
        cb->begin();

        Mesh::MeshBundleEntry v{};

        v.vertexOffset = bundle->vertexCount_;
        v.indexOffset = bundle->indexCount_;
        v.indexCount = static_cast<uint32_t>(mesh.indexCount);

        size_t v_size = mesh.vertexCount * bundle->vertexSize_;
        size_t i_size = mesh.indexCount * sizeof(uint32_t);

        const auto staging_buffer = RxCore::iVulkan()->createStagingBuffer(v_size, mesh.vertices);
        const auto staging_buffer_2 = RxCore::iVulkan()->createStagingBuffer(i_size, mesh.indices);

        cb->copyBuffer(staging_buffer, bundle->vertexBuffer_, 0,
                       bundle->vertexCount_ * bundle->vertexSize_, v_size);
        cb->copyBuffer(staging_buffer_2, bundle->indexBuffer_, 0,
                       bundle->indexCount_ * sizeof(uint32_t), i_size);

        bundle->vertexCount_ += static_cast<uint32_t>(mesh.vertexCount);
        bundle->indexCount_ += static_cast<uint32_t>(mesh.indexCount);

        cb->end();
        cb->submitAndWait();

        return meshId.set<Mesh::MeshBundleEntry>(v);
    }
#endif
    Mesh::MeshBundleEntry Mesh::addToMeshBundle(
        Mesh::MeshBundle & bundle,
        const void * vertices,
        size_t vertexCount,
        const void * indices,
        size_t indexCount)
    {
        assert(canFit(bundle, vertexCount, indexCount));

        auto cb = RxCore::iVulkan()->transferCommandPool_->createTransferCommandBuffer();
        cb->begin();

        Mesh::MeshBundleEntry v{};

        v.vertexOffset = bundle.vertexCount_;
        v.indexOffset = bundle.indexCount_;
        v.indexCount = static_cast<uint32_t>(indexCount);

        size_t v_size = vertexCount * bundle.vertexSize_;
        size_t i_size = indexCount * sizeof(uint32_t);

        const auto staging_buffer = RxCore::iVulkan()->createStagingBuffer(v_size, vertices);
        const auto staging_buffer_2 = RxCore::iVulkan()->createStagingBuffer(i_size, indices);

        cb->copyBuffer(staging_buffer, bundle.vertexBuffer_, 0,
                       bundle.vertexCount_ * bundle.vertexSize_, v_size);
        cb->copyBuffer(staging_buffer_2, bundle.indexBuffer_, 0,
                       bundle.indexCount_ * sizeof(uint32_t), i_size);

        bundle.vertexCount_ += static_cast<uint32_t>(vertexCount);
        bundle.indexCount_ += static_cast<uint32_t>(indexCount);

        cb->end();
        cb->submitAndWait();

        return v;
    }

#if 0
    Mesh::MeshBundleId addNewMeshBundleToGroup(const Mesh::MeshGroupId & mgId)
    {
        const auto * group = mgId.get<Mesh::MeshGroup>();

        const auto new_mb_id = mgId.world()
                                   .entity()
                                   .mut(mgId)
                                   .add<Mesh::MeshBundle>()
                                   .add(flecs::ChildOf, mgId);

        Mesh::MeshBundle mb{};

        mb.vertexCount_ = 0;
        mb.indexCount_ = 0;
        mb.useDescriptor_ = !group->fixedFunction;

        mb.maxVertexCount_ = static_cast<uint32_t>((group->groupBufferSize * 1024 * 1024 / group->
            vertexSize));
        mb.vertexBuffer_ = RxCore::iVulkan()->createBuffer(
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer |
            vk::BufferUsageFlagBits::eTransferDst,
            VMA_MEMORY_USAGE_GPU_ONLY, mb.maxVertexCount_ * group->vertexSize);

        mb.indexBuffer_ = RxCore::iVulkan()->createIndexBuffer(
            VMA_MEMORY_USAGE_GPU_ONLY, static_cast<uint32_t>(mb.maxVertexCount_ * sizeof(uint32_t)),
            false);

        new_mb_id.set<Mesh::MeshBundle>(std::move(mb));

        return new_mb_id;
    }
#endif

    void Mesh::createMeshBundleForGroup(const MeshGroup & group, Mesh::MeshBundle & mb)
    {
        mb.vertexCount_ = 0;
        mb.indexCount_ = 0;
        mb.useDescriptor_ = !group.fixedFunction;

        mb.maxVertexCount_ = static_cast<uint32_t>((group.groupBufferSize * 1024 * 1024 / group.
            vertexSize));
        mb.maxIndexCount_ = 3 * mb.maxVertexCount_ / 2;

        mb.vertexSize_ = static_cast<uint32_t>(group.vertexSize);

        mb.vertexBuffer_ = RxCore::iVulkan()->createBuffer(
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer |
            vk::BufferUsageFlagBits::eTransferDst,
            VMA_MEMORY_USAGE_GPU_ONLY, mb.maxVertexCount_ * group.vertexSize);

        mb.indexBuffer_ = RxCore::iVulkan()->createIndexBuffer(
            VMA_MEMORY_USAGE_GPU_ONLY, static_cast<uint32_t>(mb.maxVertexCount_ * sizeof(uint32_t)),
            false);
    }

    void Mesh::onSetMeshGroup(ecs::entity_t e, MeshGroup & g)
    {
        MeshBundle mb{};
        createMeshBundleForGroup(g, mb);
        auto newBundle = e.world().entity().mut(e)
                          .set<MeshBundle>(mb)
                          .add(flecs::ChildOf, e);
        g.currentBundle = newBundle;
    }

    void Mesh::onSetBundleFull(ecs::entity_t e, const BundleFull &)
    {
        MeshBundle mb{};
        auto g = e.get_parent<MeshGroup>(); // .get<MeshGroup>();
        auto mg = g.mut(e).get_mut<MeshGroup>();
        createMeshBundleForGroup(*mg, mb);
        auto newBundle = e.world().entity().mut(e)
                          .set<MeshBundle>(mb)
                          .add(flecs::ChildOf, g);
        mg->currentBundle = newBundle;
    }

    void Mesh::meshBundleGui(ecs::World * w, ecs::entity_t e)
    {
        auto b = w->get<MeshBundle>(e);
        if (b) {
            if (ImGui::BeginTable(
                "ComponentGui", 2,
                /*ImGuiTableFlags_Borders | */
                ImGuiTableFlags_Resizable |
                ImGuiTableFlags_Hideable)) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Value");
                //ImGui::TableHeadersRow();

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Max Vertices");
                ImGui::TableNextColumn();
                ImGui::Text("%d", b->maxVertexCount_);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Usage");
                ImGui::TableNextColumn();
                std::ostringstream stringStream;
                stringStream << (b->vertexCount_) << "/"
                    << (b->maxVertexCount_);

                ImGui::ProgressBar(
                    static_cast<float>(b->vertexCount_) /
                    static_cast<float>(b->
                        maxVertexCount_),
                    ImVec2(-1, 0),
                    stringStream.str().c_str());

                ImGui::EndTable();
            }
        }
    }

    void Mesh::registerPlugin(ecs::World * world)
    {
#if 0
        flecs::module<Mesh>(ecs);

        flecs::entity c;
        ecs.component<MeshGroup>();
        const flecs::entity bundleComponent = ecs.component<MeshBundle>();
        ecs.component<MeshBundleEntry>();
        //ecs.component<SubMesh>();
        ecs.component<BundleFull>();

        world->set<RxEngine::ComponentGui>(
            world->getComponentId<MeshBundle>(),
            {
                .editor = meshBundleGui
            }
        );
#endif
#if 1
#if 0
            ecs.system<MeshGroup>()

               .kind(flecs::OnLoad)
               .each([&](flecs::entity e, const MeshGroup & g)
                   {
                       auto childCount = 0;
                       for (auto children: e.children()) {
                           for (auto i: children) {
                               auto c = children.entity(i);
                               if (!c.has<BundleFull>() && c.has<MeshBundle>()) {
                                   childCount++;
                               }
                           }
                       }

                       if (childCount > 0) {
                           return;
                       }
                       auto mb = createMeshBundleForGroup(g);
                       auto newBundle = e.world().entity().mut(e)
                           .set<MeshBundle>(mb)
                           .add(flecs::ChildOf, e);
                       
                   }
               );
#endif
        struct AddMeshGroup
        {
            ecs::entity_t meshGroup;
        };
        struct BunudleFull
        {
            ecs::entity_t bundle;
        };

        auto addMeshGroup = world->createStream<AddMeshGroup>();
        auto bundleFullStream = world->createStream<BundleFull>();

        world->createSystem()
             .withStream(addMeshGroup)
             .before<ecs::Pipeline::EndInit>()
             .execute<AddMeshGroup>([](ecs::World * w, const AddMeshGroup * amg)
                 {
                     MeshBundle mb{};
                     auto mg = w->get<MeshGroup>(amg->meshGroup);

                     createMeshBundleForGroup(*mg, mb);
                     auto ne = w->newEntity();
                     w->set<MeshBundle>(ne, std::move(mb));
                     //auto newBundle = e.world().entity().mut(e)
                     //  .set<MeshBundle>(mb)
                     //.add(flecs::ChildOf, e);
                     mg.currentBundle = ne;
                     return true;
                 }
             );

        world->createSystem()
             .withStream(st)
             .execute<BundleFull>([](World * w, const BundleFull * mg) {});

        world->createSystem()
             .withQuery<MeshGroup>()


        ecs.system<MeshGroup>("onSetMeshGroup")
           .kind(flecs::OnSet)
           .each(onSetMeshGroup);

        ecs.system<BundleFull>("onSetBundleFull")
           .kind(flecs::OnAdd)
           .each(onSetBundleFull);
#endif
#if 0
            ecs.system<Mesh::MeshBundle>()
               .kind(flecs::OnLoad)
               .each([](flecs::entity e, MeshBundle & g)
                   {
                       e.add<BundleFull>();
                       //e.destruct();
                   }
               );
#endif
#if 0
            ecs.system<MeshBundle>()
               .each([](flecs::entity e, MeshBundle & mb)
                   {
                       for (auto children: e.children()) {
                           for (auto i: children) {
                               if (children.entity(i).has<MeshBundleEntry>()) {
                                   return;
                               }
                           }
                       }
                       e.destruct();
                   }
               );

            ecs.system<MeshBundleEntry>()
               .kind(flecs::OnRemove)
               .each([](flecs::entity e, MeshBundleEntry & mbe)
                   {
                       auto p = e.get_parent<MeshBundle>();
                       for (auto children: p.children()) {
                           for (auto i: children) {
                               if (children.entity(i).has<MeshBundleEntry>()) {
                                   return;
                               }
                           }
                       }
                   }
               );
#endif
    }
#if 0
    Mesh::MeshBundleEntryId addMeshToGroup(const Mesh::MeshGroupId & mgId,
                                           const Mesh::MeshToBundle & mesh)
    {
        auto * group = mgId.get_mut<Mesh::MeshGroup>();

        auto bundleId = group->activeBundle;

        if (bundleId.world() && bundleId.is_valid()) {
            auto * bundle = group->activeBundle.get_mut<Mesh::MeshBundle>();
            if (canFit(bundle, mesh)) {
                return addToMeshBundle(group->activeBundle, bundle, mesh);
            }
            bundleId = flecs::entity::null();
        }
        bundleId = addNewMeshBundleToGroup(mgId);
        group->activeBundle = bundleId;
        //        mgId.patch<Mesh::MeshGroup>([=](Mesh::MeshGroup & g)
        //      {
        //        g.activeBundle = bundleId;
        //  });
        auto * bundle = group->activeBundle.mut(mgId).get_mut<Mesh::MeshBundle>();
        return addToMeshBundle(group->activeBundle.mut(mgId), bundle, mesh);
    }
#endif
#if 0
    void ensureMeshBundleDescriptorSet(const Mesh::MeshBundleId & mbId,
                                       const RxCore::DescriptorPoolTemplate & poolTemplate,
                                       vk::DescriptorSetLayout layout)
    {
        auto * mesh_bundle = mbId.get_mut<Mesh::MeshBundle>();
        if (!mesh_bundle->useDescriptor_)
            return;
        if (mesh_bundle->descriptorSet_)
            return;

        mesh_bundle->descriptorSet_ = RxCore::JobManager::threadData().getDescriptorSet(
            poolTemplate, layout);
        mesh_bundle->descriptorSet_->updateDescriptor(0, vk::DescriptorType::eStorageBuffer,
                                                      mesh_bundle->vertexBuffer_);
    }
#endif
}
