//
// Created by shane on 24/02/2021.
//

#ifndef RX_MESHBUNDLE_H
#define RX_MESHBUNDLE_H

#include <memory>
#include <vector>
#include "Vulkan/VertexBuffer.h"
#include "Vulkan/IndexBuffer.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Util.h"

namespace RxEngine
{
    struct MeshBundleEntry
    {
        uint32_t indexCount;
        uint32_t indexOffset;
        uint32_t vertexOffset;
    };

    struct MeshToBundle
    {
        void * vertices;
        size_t vertexCount;
        void * indices;
        size_t indexCount;
    };

    class MeshBundle
    {
    public:
        MeshBundle(size_t vertexSize, size_t allocateSize, bool useDescriptor = true);

        virtual ~MeshBundle();

        RX_NO_COPY_NO_MOVE(MeshBundle)

        uint32_t add(const MeshToBundle & mesh);
        bool canFit(const std::vector<MeshToBundle> & meshes);

        MeshBundleEntry getEntry(uint32_t ix) const;
        const std::shared_ptr<RxCore::Buffer> & getVertexBuffer() const;
        const std::shared_ptr<RxCore::IndexBuffer> & getIndexBuffer() const;

        void bindDescriptor(const std::shared_ptr<RxCore::DescriptorSet> & ds, uint32_t binding);
        void ensureDescriptorSet(
            const RxCore::DescriptorPoolTemplate & poolTemplate,
            vk::DescriptorSetLayout layout);

        std::shared_ptr<RxCore::DescriptorSet> getDescriptorSet() const;
        bool isUseDescriptor() const;

    private:
        std::vector<MeshBundleEntry> entries_;
        std::shared_ptr<RxCore::Buffer> vertexBuffer_;
        std::shared_ptr<RxCore::IndexBuffer> indexBuffer_;
        std::shared_ptr<RxCore::DescriptorSet> descriptorSet_;

        uint32_t vertexSize_;

        uint32_t vertexCount_;
        uint32_t indexCount_;

        uint32_t maxIndexCount_;
        uint32_t maxVertexCount_;

        bool useDescriptor_;
    };
}

#endif //RX_MESHBUNDLE_H
