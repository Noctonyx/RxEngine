#pragma once

#include "RxECS.h"
//#include <flecs.h>
#include "Vulkan/IndexBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"

namespace RxCore
{
    struct DescriptorPoolTemplate;
}

namespace RxEngine
{
    struct Mesh
    {
        //using MeshGroupId = flecs::entity;
        //using MeshBundleId = flecs::entity;
        //using MeshBundleEntryId = flecs::entity;

        struct MeshGroup
        {
            uint32_t groupBufferSize;
            size_t vertexSize;
            bool fixedFunction;
            ecs::entity_t currentBundle{};
        };

        struct MeshBundle
        {
            std::shared_ptr<RxCore::Buffer> vertexBuffer_{};
            std::shared_ptr<RxCore::IndexBuffer> indexBuffer_{};
            std::shared_ptr<RxCore::DescriptorSet> descriptorSet_{};

            uint32_t vertexSize_{};

            uint32_t vertexCount_{};
            uint32_t indexCount_{};

            uint32_t maxIndexCount_{};
            uint32_t maxVertexCount_{};

            bool useDescriptor_{};
#if 0
            ~MeshBundle()
            {
                vertexBuffer_.reset();
                indexBuffer_.reset();
                descriptorSet_.reset();
            }
#endif
        };

        struct MeshBundleEntry
        {
            uint32_t indexCount;
            uint32_t indexOffset;
            uint32_t vertexOffset;
        };
#if 0
        struct MeshToBundle
        {
            const void * vertices;
            size_t vertexCount;
            const void * indices;
            size_t indexCount;
        };
#endif
        //struct SubMesh {};

#if 0
        static MeshGroupId createMeshGroup(
            flecs::world & ecs,
            uint32_t groupBufferSize,
            size_t vertexSize,
            bool fixedFunction);
#endif
#if 0
        Mesh::MeshBundleEntryId addMeshToGroup(
            const Mesh::MeshGroupId& mgId,
            const Mesh::MeshToBundle& mesh);
#endif
        static bool canFit(const MeshBundle & bundle, size_t vertexCount, size_t indexCount);

#if 0
        static MeshBundleEntryId addToMeshBundle(
            const MeshBundleId & mbId,
            MeshBundle * bundle,
            const MeshToBundle & mesh);
#endif
        static MeshBundleEntry addToMeshBundle(
            MeshBundle & bundle,
            const void * vertices,
            size_t vertexCount,
            const void * indices,
            size_t indexCount);

        //Mesh::MeshBundleId addNewMeshBundleToGroup(const Mesh::MeshGroupId & mgId);
#if 0
        void ensureMeshBundleDescriptorSet(
            const MeshBundleId & mbId,
            const RxCore::DescriptorPoolTemplate & poolTemplate,
            vk::DescriptorSetLayout layout);
#endif
        struct BundleFull {};

        static void createMeshBundleForGroup(const MeshGroup & group, MeshBundle & mb);
        static void onSetMeshGroup(ecs::entity_t e, MeshGroup & g);
        static void onSetBundleFull(ecs::entity_t e, const BundleFull &);

        static void meshBundleGui(ecs::World* w, ecs::entity_t e);
        static void registerPlugin(ecs::World* world);
        //explicit Mesh(flecs::world & ecs);
    };
}
