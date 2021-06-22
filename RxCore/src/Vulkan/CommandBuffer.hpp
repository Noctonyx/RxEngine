#pragma once

#include <set>
#include <vector>
#include <memory>
#include <map>
#include "Device.h"
#include "CommandPool.hpp"
#include "Vulk.hpp"
#include "Util.h"

namespace RxCore
{
    class Buffer;
    class Pipeline;
    class RenderPass;
    //class PipelineLayout;
    class DescriptorSet;
    class FrameBuffer;

    class CommandBuffer
    {
        friend class Device;

    public:
        CommandBuffer(RxCore::Device * device, VkCommandBuffer handle, std::shared_ptr<CommandPool> pool)
            : device_(device)
            , handle_(handle)
            , commandPool_(std::move(pool))
        {}

        RX_NO_COPY_NO_MOVE(CommandBuffer);

        virtual ~CommandBuffer()
        {
            device_->freeCommandBuffer(this);
            //Device::VkDevice().freeCommandBuffers(commandPool_->GetHandle(), 1, &handle_);
        }

        void useLayout(VkPipelineLayout layout)
        {
            currentLayout_ = layout;
        }

        void begin();
        void end();

        void beginRenderPass(
            VkRenderPass renderPass,
            std::shared_ptr<FrameBuffer> fb,
            const VkExtent2D extent,
            const std::vector<VkClearValue> & clearValues);

        void EndRenderPass();

        void setViewport(
            float x,
            float y,
            float width,
            float height,
            float minDepth,
            float maxDepth) const;

        void setScissor(VkRect2D rect) const;
        void clearScissor() const;

        VkImageMemoryBarrier ImageBarrier(
            VkImage image,
            VkAccessFlags srcAccessMask,
            VkAccessFlags destAccessMask,
            VkImageLayout oldLayout,
            VkImageLayout newLayout);

        void PipelineBarrierImage(
            VkImage image,
            VkPipelineStageFlags srcStageMask,
            VkAccessFlags srcAccessMask,
            VkImageLayout oldLayout,
            VkPipelineStageFlags destStageMask,
            VkAccessFlags destAccessMask,
            VkImageLayout newLayout);

        void bindPipeline(VkPipeline pipeline);
        void bindVertexBuffer(std::shared_ptr<Buffer> buffer);
        void bindIndexBuffer(std::shared_ptr<IndexBuffer> buffer, uint64_t offset = 0);

        void DrawIndexed(
            uint32_t indexCount,
            uint32_t instanceCount,
            uint32_t firstIndex,
            int32_t vertexOffset,
            uint32_t firstInstance);

        void Draw(
            uint32_t vertexCount,
            uint32_t instanceCount,
            uint32_t firstVertex,
            uint32_t firstInstance);
#if 0
        void PushBuffer(
            const PipelineLayout & layout,
            uint32_t binding,
            std::shared_ptr<Buffer> buffer,
            VkDescriptorType type,
            uint32_t set = 0);
#endif
        void BindDescriptorSet(
            // VkPipelineLayout pipelineLayout,
            uint32_t firstSet,
            std::shared_ptr<DescriptorSet> usedSet);

        void pushConstant(
            // VkPipelineLayout layout,
            VkShaderStageFlags shaderFlags,
            uint32_t offset,
            uint32_t size,
            const void * ptr) const;

        //operator VkCommandBuffer() const { return handle_; };
        [[nodiscard]] VkCommandBuffer Handle() const
        {
            return handle_;
        }

        [[nodiscard]] bool wasStarted() const
        {
            return started_;
        }

    protected:
        RxCore::Device * device_;
        VkCommandBuffer handle_;
        std::set<std::shared_ptr<Buffer>> buffers_;
        //std::set<std::shared_ptr<Pipeline>> pipelines_;
        std::set<std::shared_ptr<DescriptorSet>> descriptorSets_;
        std::set<std::shared_ptr<FrameBuffer>> frameBuffers_;
        std::shared_ptr<CommandPool> commandPool_;
        bool started_ = false;

        VkPipelineLayout currentLayout_;
    };

    class PrimaryCommandBuffer final : public CommandBuffer
    {
    public:
        PrimaryCommandBuffer(RxCore::Device * device,const VkCommandBuffer & handle, std::shared_ptr<CommandPool> pool)
            : CommandBuffer(device, handle, std::move(pool))
        {}

        ~PrimaryCommandBuffer() override
        {
            secondaries2_.clear();
            secondaries_.clear();
        }

        RX_NO_COPY_NO_MOVE(PrimaryCommandBuffer)

        void addSecondaryBuffer(std::shared_ptr<SecondaryCommandBuffer> secondary,
                                uint16_t sequence);
        void executeSecondaries(uint16_t sequence);
        void executeSecondary(const std::shared_ptr<SecondaryCommandBuffer> & secondary);

    protected:
        std::map<uint16_t, std::vector<std::shared_ptr<SecondaryCommandBuffer>>> secondaries_;
        std::vector<std::shared_ptr<SecondaryCommandBuffer>> secondaries2_;
    };

    class TransferCommandBuffer final : public CommandBuffer
    {
    public:
        TransferCommandBuffer(RxCore::Device * device, const VkCommandBuffer & handle, std::shared_ptr<CommandPool> pool)
            : CommandBuffer(device, handle, std::move(pool))
        {}

        RX_NO_COPY_NO_MOVE(TransferCommandBuffer)

        void copyBuffer(
            std::shared_ptr<Buffer> source,
            std::shared_ptr<Buffer> dest,
            VkDeviceSize srcOffset,
            VkDeviceSize destOffset,
            VkDeviceSize size);

        void copyBufferToImage(
            std::shared_ptr<Buffer> source,
            std::shared_ptr<Image> dest,
            VkExtent3D extent,
            uint32_t layerCount,
            uint32_t baseArrayLayer,
            uint32_t mipLevel);

        void imageTransition(std::shared_ptr<Image> dest,
                             VkImageLayout destLayout,
                             uint32_t mipLevel = 0);

        void submitAndWait();

    protected:
        std::set<std::shared_ptr<Buffer>> buffers_;
        std::set<std::shared_ptr<Image>> images_;
    };

    class SecondaryCommandBuffer : public CommandBuffer
    {
    public:
        SecondaryCommandBuffer(
            RxCore::Device * device,
            VkCommandBuffer handle,
            std::shared_ptr<CommandPool> pool)
            : CommandBuffer(device, handle, std::move(pool))
        {}

        RX_NO_COPY_NO_MOVE(SecondaryCommandBuffer)

        void begin(VkRenderPass renderPass, uint32_t subPass);

    protected:
        //VkRenderPass renderPass_;
        //uint32_t subPass_;
    };
} // namespace RXCore
