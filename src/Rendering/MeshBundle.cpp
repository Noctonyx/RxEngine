//
// Created by shane on 24/02/2021.
//

#include <memory>
#include <Vulkan/Device.h>
#include <RXCore.h>
#include "MeshBundle.h"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/ThreadResources.h"

#define RX_INVALID_ID  std::numeric_limits<uint32_t>::max()

namespace RxEngine
{
    const std::shared_ptr<RxCore::Buffer> & MeshBundle::getVertexBuffer() const
    {
        return vertexBuffer_;
    }

    const std::shared_ptr<RxCore::IndexBuffer> & MeshBundle::getIndexBuffer() const
    {
        return indexBuffer_;
    }

    MeshBundleEntry MeshBundle::getEntry(uint32_t ix) const
    {
        return entries_[ix];
    }

    void MeshBundle::bindDescriptor(const std::shared_ptr<RxCore::DescriptorSet> & ds, uint32_t binding)
    {
        assert(useDescriptor_);
        ds->updateDescriptor(binding, vk::DescriptorType::eStorageBuffer, vertexBuffer_);
    }

    MeshBundle::~MeshBundle()
    {
        vertexBuffer_.reset();
        indexBuffer_.reset();
        descriptorSet_.reset();
    }

    void MeshBundle::ensureDescriptorSet(
        const RxCore::DescriptorPoolTemplate & poolTemplate,
        vk::DescriptorSetLayout layout)
    {
        if (!useDescriptor_) {
            return;
        }

        if (descriptorSet_) {
            return;
        }

        descriptorSet_ = RxCore::threadResources.getDescriptorSet(poolTemplate, layout);

        descriptorSet_->updateDescriptor(0, vk::DescriptorType::eStorageBuffer, vertexBuffer_);
    }

    std::shared_ptr<RxCore::DescriptorSet> MeshBundle::getDescriptorSet() const
    {
        assert(useDescriptor_);
        assert(descriptorSet_);

        return descriptorSet_;
    }

    bool MeshBundle::isUseDescriptor() const
    {
        return useDescriptor_;
    }

    MeshBundle::MeshBundle(size_t vertexSize, size_t allocateSize, bool useDescriptor)
        : vertexSize_(static_cast<uint32_t>(vertexSize))
        , vertexCount_(0)
        , indexCount_(0)
        , useDescriptor_(useDescriptor)
    {
        maxVertexCount_ = static_cast<uint32_t>((allocateSize / vertexSize));
        maxIndexCount_ = maxVertexCount_;

        vertexBuffer_ = RxCore::iVulkan()->createBuffer(
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer |
            vk::BufferUsageFlagBits::eTransferDst,
            VMA_MEMORY_USAGE_GPU_ONLY, maxVertexCount_ * vertexSize);

        indexBuffer_ = RxCore::iVulkan()->createIndexBuffer(
            VMA_MEMORY_USAGE_GPU_ONLY, static_cast<uint32_t>(maxVertexCount_ * sizeof(uint32_t)), false);
    }

    uint32_t MeshBundle::add(const MeshToBundle & mesh)
    {
        if (mesh.vertexCount + vertexCount_ > maxVertexCount_) {
            return RX_INVALID_ID;
        }

        if (mesh.indexCount + indexCount_ > maxIndexCount_) {
            return RX_INVALID_ID;
        }

        auto cb = RxCore::iVulkan()->transferCommandPool_->createTransferCommandBuffer();
        cb->begin();

        uint32_t new_ix = static_cast<uint32_t>(entries_.size());
        auto & v = entries_.emplace_back();
        v.vertexOffset = vertexCount_;
        v.indexOffset = indexCount_;
        v.indexCount = static_cast<uint32_t>(mesh.indexCount);

        size_t v_size = mesh.vertexCount * vertexSize_;
        size_t i_size = mesh.indexCount * sizeof(uint32_t);

        auto staging_buffer = RxCore::iVulkan()->createStagingBuffer(v_size, mesh.vertices);
        auto staging_buffer_2 = RxCore::iVulkan()->createStagingBuffer(i_size, mesh.indices);

        cb->copyBuffer(staging_buffer, vertexBuffer_, 0, vertexCount_ * vertexSize_, v_size);
        cb->copyBuffer(staging_buffer_2, indexBuffer_, 0, indexCount_ * sizeof(uint32_t), i_size);

        vertexCount_ += static_cast<uint32_t>(mesh.vertexCount);
        indexCount_ += static_cast<uint32_t>(mesh.indexCount);

        cb->end();
        cb->submitAndWait();

        return new_ix;
    }

    bool MeshBundle::canFit(const std::vector<MeshToBundle> & meshes)
    {
        uint32_t ixCount = 0;
        uint32_t vxCount = 0;

        for (auto & m: meshes) {
            ixCount += static_cast<uint32_t>(m.indexCount);
            vxCount += static_cast<uint32_t>(m.vertexCount);
        }
        if (vxCount + vertexCount_ > maxVertexCount_) {
            return false;
        }

        if (ixCount + indexCount_ > maxIndexCount_) {
            return false;
        }

        return true;
    }
}
