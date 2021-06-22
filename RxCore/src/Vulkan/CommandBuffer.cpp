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

#include <algorithm>
#include "CommandBuffer.hpp"
#include "Buffer.hpp"
#include "VertexBuffer.h"
#include "IndexBuffer.hpp"
#include "DescriptorSet.hpp"
#include "FrameBuffer.h"
#include "Queue.hpp"
#include "optick/optick.h"
#include "Image.hpp"

namespace RxCore
{
    void CommandBuffer::begin()
    {
        OPTICK_EVENT()
        started_ = true;
        buffers_.clear();
        descriptorSets_.clear();

        VkCommandBufferBeginInfo cbi{};
        cbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        vkBeginCommandBuffer(handle_, &cbi);
    }

    void SecondaryCommandBuffer::begin(VkRenderPass renderPass, uint32_t subPass)
    {
        OPTICK_EVENT()
        started_ = true;
        buffers_.clear();
        descriptorSets_.clear();

        VkCommandBufferBeginInfo cbbi{};
        cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkCommandBufferInheritanceInfo cbii{};
        cbii.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        cbii.renderPass = renderPass;
        cbii.subpass = subPass;
        cbbi.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        cbbi.pInheritanceInfo = &cbii;

        vkBeginCommandBuffer(handle_, &cbbi);
    }

    void CommandBuffer::end()
    {
        OPTICK_EVENT();

        vkEndCommandBuffer(handle_);
    }

    void CommandBuffer::beginRenderPass(
        VkRenderPass renderPass,
        std::shared_ptr<FrameBuffer> fb,
        const VkExtent2D extent,
        const std::vector<VkClearValue> & clearValues)
    {
        OPTICK_EVENT()
        VkRenderPassBeginInfo rpbi{};

        rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpbi.renderPass = renderPass;
        rpbi.framebuffer = fb->Handle();
        rpbi.renderArea = {{0, 0}, extent};
        rpbi.clearValueCount = static_cast<uint32_t>(clearValues.size());
        rpbi.pClearValues = clearValues.data();

        frameBuffers_.emplace(std::move(fb));
        vkCmdBeginRenderPass(handle_, &rpbi, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
    }

    void CommandBuffer::setViewport(
        float x,
        float y,
        float width,
        float height,
        float minDepth,
        float maxDepth) const
    {
        OPTICK_EVENT()

        VkViewport vp = {x, y, width, height, minDepth, maxDepth};
        vkCmdSetViewport(handle_, 0, 1, &vp);
    }

    void CommandBuffer::setScissor(VkRect2D rect) const
    {
        OPTICK_EVENT()

        VkRect2D sc = {rect};
        vkCmdSetScissor(handle_, 0, 1, &sc);
    }

    void CommandBuffer::clearScissor() const
    {
        VkRect2D sc = {};
        vkCmdSetScissor(handle_, 0, 1, &sc);
    }

    void CommandBuffer::BindDescriptorSet(
        uint32_t firstSet,
        std::shared_ptr<DescriptorSet> usedSet)
    {
        auto offsets = usedSet->getOffsets();

        vkCmdBindDescriptorSets(handle_, VK_PIPELINE_BIND_POINT_GRAPHICS, currentLayout_, firstSet,
                                1, &usedSet->handle, static_cast<uint32_t>(offsets.size()),
                                offsets.data());

        descriptorSets_.emplace(std::move(usedSet));
    }

    void CommandBuffer::pushConstant(
        VkShaderStageFlags shaderFlags,
        uint32_t offset,
        uint32_t size,
        const void * ptr) const
    {
        vkCmdPushConstants(handle_, currentLayout_, shaderFlags, offset, size, ptr);
        //        handle_.pushConstants(currentLayout_, shaderFlags, offset, size, ptr);
    }

    void CommandBuffer::bindPipeline(VkPipeline pipeline)
    {
        vkCmdBindPipeline(handle_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        //handle_.bindPipeline(VkPipelineBindPoint::eGraphics, pipeline);
    }

    void CommandBuffer::bindVertexBuffer(std::shared_ptr<Buffer> buffer)
    {
        auto b = std::vector{buffer->handle()};
        auto o = std::vector{0ULL};

        vkCmdBindVertexBuffers(handle_, 0, static_cast<uint32_t>(b.size()), b.data(), o.data());
        //handle_.bindVertexBuffers(0, {buffer->handle()}, {0});
        buffers_.emplace(std::move(buffer));
    }

    void CommandBuffer::DrawIndexed(
        uint32_t indexCount,
        uint32_t instanceCount,
        uint32_t firstIndex,
        int32_t vertexOffset,
        uint32_t firstInstance)
    {
        vkCmdDrawIndexed(handle_, indexCount, instanceCount, firstIndex, vertexOffset,
                         firstInstance);
        //handle_.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void CommandBuffer::bindIndexBuffer(std::shared_ptr<IndexBuffer> buffer, uint64_t offset)
    {
        if (buffer->is16Bit_) {
            vkCmdBindIndexBuffer(handle_, buffer->handle(), offset, VK_INDEX_TYPE_UINT16);
        } else {
            vkCmdBindIndexBuffer(handle_, buffer->handle(), offset, VK_INDEX_TYPE_UINT32);
        }
        buffers_.emplace(std::move(buffer));
    }

    void CommandBuffer::EndRenderPass()
    {
        vkCmdEndRenderPass(handle_);
    }
#if 0
    VkImageMemoryBarrier CommandBuffer::ImageBarrier(
        VkImage image,
        VkAccessFlags srcAccessMask,
        VkAccessFlags destAccessMask,
        VkImageLayout oldLayout,
        VkImageLayout newLayout)
    {
        VkImageSubresourceRange srr = {
            VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS
        };


        auto ib = VkImageMemoryBarrier();
        ib.setSrcAccessMask(srcAccessMask)
          .setDstAccessMask(destAccessMask)
          .setOldLayout(oldLayout)
          .setNewLayout(newLayout)
          .setImage(image)
          .setSubresourceRange(srr); //.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);

        return ib;
    }
#endif
    void CommandBuffer::Draw(
        uint32_t vertexCount,
        uint32_t instanceCount,
        uint32_t firstVertex,
        uint32_t firstInstance)
    {
        vkCmdDraw(handle_, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void PrimaryCommandBuffer::executeSecondaries(uint16_t sequence)
    {
        OPTICK_EVENT("Execute 2nd")
        std::vector<VkCommandBuffer> bufs(secondaries_[sequence].size());

        if (bufs.empty()) {
            return;
        }

        std::ranges::transform(
            secondaries_[sequence], bufs.begin(), [](auto & b)
            {
                return b->Handle();
            });
        vkCmdExecuteCommands(handle_, static_cast<uint32_t>(bufs.size()), bufs.data());
    }

    void PrimaryCommandBuffer::executeSecondary(
        const std::shared_ptr<SecondaryCommandBuffer> & secondary)
    {
        std::vector<VkCommandBuffer> bufs = {secondary->Handle()};

        vkCmdExecuteCommands(handle_, static_cast<uint32_t>(bufs.size()), bufs.data());

        secondaries2_.push_back(secondary);
    }

    void PrimaryCommandBuffer::addSecondaryBuffer(
        std::shared_ptr<SecondaryCommandBuffer> secondary,
        uint16_t sequence)
    {
        if (!secondary || !secondary->wasStarted()) {
            return;
        }
        if (!secondaries_.contains(sequence)) {
            secondaries_[sequence] = {};
        }
        secondaries_[sequence].push_back(std::move(secondary));
    }

    void TransferCommandBuffer::copyBuffer(
        std::shared_ptr<Buffer> source,
        std::shared_ptr<Buffer> dest,
        VkDeviceSize srcOffset,
        VkDeviceSize destOffset,
        VkDeviceSize size)
    {
        VkBufferCopy bc{srcOffset, destOffset, size};

        vkCmdCopyBuffer(handle_, source->handle(), dest->handle(), 1, &bc);
        (void) buffers_.emplace(std::move(source));
        (void) buffers_.emplace(std::move(dest));
    }

    void TransferCommandBuffer::submitAndWait()
    {
        auto queue = device_->getTransferQueue();
        //auto queue = RxCore::iVulkan()->transferQueue_;

        VkSubmitInfo si{};
        si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.commandBufferCount = 1;
        si.pCommandBuffers = &handle_;

        auto fence = device_->createFence();
        vkQueueSubmit(queue->GetHandle(), 1, &si, fence);

        const auto result = device_->waitForFence(fence);
        assert(result == VK_SUCCESS);

        device_->destroyFence(fence);
        buffers_.clear();
    }

    void TransferCommandBuffer::copyBufferToImage(
        std::shared_ptr<Buffer> source,
        std::shared_ptr<Image> dest,
        VkExtent3D extent,
        uint32_t layerCount,
        uint32_t baseArrayLayer,
        uint32_t mipLevel)
    {
        VkBufferImageCopy bc{
            0,
            0,
            0,
            {VK_IMAGE_ASPECT_COLOR_BIT, mipLevel, baseArrayLayer, layerCount},
            {0, 0, 0},
            extent
        };

        vkCmdCopyBufferToImage(handle_,
                               source->handle(),
                               dest->handle(),
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &bc);

        images_.emplace(std::move(dest));
        buffers_.emplace(std::move(source));
    }

    void TransferCommandBuffer::imageTransition(
        std::shared_ptr<Image> dest,
        VkImageLayout destLayout,
        uint32_t mipLevel)
    {
        //VkPipelineStageFlags src_stage, dest_stage;

        VkImageMemoryBarrier imb{};
        imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imb.image = dest->handle();
        imb.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imb.subresourceRange.baseMipLevel = mipLevel;
        imb.subresourceRange.levelCount = 1;
        imb.subresourceRange.baseArrayLayer = 0;
        imb.subresourceRange.layerCount = 1;
        imb.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imb.newLayout = destLayout;
        imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;


        std::vector<VkImageMemoryBarrier> vi = {imb};

        vkCmdPipelineBarrier(handle_, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imb);
        images_.emplace(std::move(dest));
    }
} // namespace RXCore
