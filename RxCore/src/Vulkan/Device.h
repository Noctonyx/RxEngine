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

#pragma once

#include <memory>
#include <optional>
#include <unordered_map>
#include "Vulk.hpp"
#include "SDL.h"
#include "Hasher.h"

namespace RxCore
{
    class VertexBuffer;
    class IndexBuffer;
    class Queue;
    class CommandPool;
    class Buffer;
    class Image;
    class DescriptorPool;
    class CommandPool;
    class CommandBuffer;
    class Shader;
    class ImageView;

    struct MemHeapStatus
    {
        uint64_t budget;
        uint64_t usage;
    };

    class Device
    {
    public:
        Device(SDL_Window * window);

        Device(const Device & other) = delete;
        Device & operator=(const Device & other) = delete;
        Device & operator=(Device && other) = delete;

        ~Device();

    public:
#if 0
        static Device * Context()
        {
            return context_;
        }

        static VkDevice VkDevice()
        {
            return context_->getDevice();
        }
#endif
        void WaitIdle() const;

        std::shared_ptr<DescriptorPool> CreateDescriptorPool(
            std::vector<VkDescriptorPoolSize> poolSizes,
            uint32_t max);

        std::shared_ptr<CommandPool> CreateGraphicsCommandPool();


        void freeCommandBuffer(CommandBuffer * buf);
        void destroyDescriptorPool(DescriptorPool * pool);

        //------- Surface
        //std::unique_ptr<SwapChain> createSwapChain();
        [[nodiscard]] uint32_t getPresentQueueFamily() const;
        //void updateSurfaceCapabilities();

       
    protected:
        //void getSurfaceDetails();
        //void selectSurfaceFormat();
        //void selectPresentationMode();
        //void selectPresentationQueueFamily();

    public:
        [[nodiscard]] VkSemaphore createSemaphore() const;
        void destroySemaphore(VkSemaphore s) const;
        [[nodiscard]] std::shared_ptr<Queue> getTransferQueue() const;

        // --------------------------------
        VkFence createFence() const;
        void destroyFence(VkFence f) const;
        VkResult waitForFence(VkFence f) const;
        VkResult getFenceStatus(VkFence f) const;

        // ---------------------------------
        std::shared_ptr<Image> createImage(
            VkFormat format,
            VkExtent3D extent,
            uint32_t mipLevels,
            uint32_t layers,
            VkImageUsageFlags usage,
            VkImageType type = VK_IMAGE_TYPE_2D);
        void destroyImage(Image * image) const;

        [[nodiscard]] std::shared_ptr<ImageView> createImageView(
            const std::shared_ptr<Image>& image,
            VkImageViewType viewType,
            VkImageAspectFlagBits aspect,
            uint32_t baseArrayLayer = 0,
            uint32_t layerCount = VK_REMAINING_ARRAY_LAYERS) const;
        void destroyImageView(ImageView * image) const;
#if 0
        std::shared_ptr<Image> Create2DImage(
            VkFormat format,
            VkExtent2D extent,
            uint32_t layers,
            VkImageUsageFlags usage) const;

        std::shared_ptr<Image> CreateImage(
            VkFormat format,
            VkExtent3D extent,
            uint32_t layers,
            VkImageUsageFlags usage,
            VkImageType type = VkImageType::e2D) const;

        std::shared_ptr<Image> createImageWithoutMemory(
            VkFormat format,
            VkExtent3D extent,
            uint32_t layers,
            VkImageUsageFlags usage,
            VkImageType type = VkImageType::e2D, uint32_t mipLevels = 1) const;

        std::shared_ptr<Image> Create2DImage(
            VkFormat format,
            uint32_t width,
            uint32_t height)
        const;

        std::shared_ptr<Image> Create2DImage(
            VkFormat format,
            uint32_t width,
            uint32_t height,
            uint8_t * pixels,
            uint32_t size) const;
#endif
        std::shared_ptr<Shader> createShader(const std::vector<uint32_t> & bytes) const;

        VkFormat GetDepthFormat(bool checkSamplingSupport) const;

        void transferBuffer(
            std::shared_ptr<Buffer> src,
            std::shared_ptr<Buffer> dst,
            size_t size,
            size_t srcOffset = 0,
            size_t destOffset = 0) const;
        void transitionImageLayout(
            const std::shared_ptr<Image> & image,
            VkImageLayout dstLayout) const;

        void transferBufferToImage(
            std::shared_ptr<Buffer> src,
            std::shared_ptr<Image> dst,
            VkExtent3D extent,
            VkImageLayout destLayout,
            uint32_t layerCount,
            uint32_t baseArrayLayer, uint32_t mipLevel = 0) const;

        [[nodiscard]] VkDevice getDevice() const
        {
            return handle_;
        };

        uint64_t getBufferAddress(const Buffer * buffer) const;
#if 0
        std::shared_ptr<Memory> allocateMemory(
            const VkMemoryPropertyFlags memFlags,
            const VkMemoryRequirements & memReq) const;
#endif
        void getMemBudget(std::vector<MemHeapStatus> & heaps) const;
        // =----  Creating objects

        VkSampler createSampler(const VkSamplerCreateInfo & sci);
        VkDescriptorSetLayout createDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo & dslci);
        VkPipelineLayout createPipelineLayout(const VkPipelineLayoutCreateInfo & plci);

        std::shared_ptr<CommandPool> createCommandPool(uint32_t index);
        // std::shared_ptr<Buffer> createUniformBuffer(MemoryType memType, const uint64_t size, void * data);
        std::shared_ptr<Buffer> createStagingBuffer(size_t size, const void * data);
        std::shared_ptr<Buffer> createBuffer(
            const VkBufferUsageFlags & flags,
            VmaMemoryUsage memType,
            VkDeviceSize size,
            void * data = nullptr);

        void destroyBuffer(VkBuffer buffer) const;
        void destroyCommandPool(VkCommandPool commandPool) const ;

#if 0
        std::shared_ptr<IndexBuffer> createIndexBuffer(
            VmaMemoryUsage memType,
            uint32_t indexCount,
            bool is16) const;
#endif
        std::shared_ptr<IndexBuffer> createIndexBuffer(
            VmaMemoryUsage memType,
            uint32_t indexCount,
            bool is16,
            void * data = nullptr);
#if 0
        std::shared_ptr<VertexBuffer> createVertexBuffer(
            VmaMemoryUsage memType,
            uint32_t vertexCount,
            uint32_t vertexSize) const;
#endif
        std::shared_ptr<VertexBuffer> createVertexBuffer(
            VmaMemoryUsage memType,
            uint32_t vertexCount,
            uint32_t vertexSize,
            void * data = nullptr);

        // =---- Getting Information

        [[nodiscard]] VkDeviceSize getUniformBufferAlignment(VkDeviceSize size) const;
        [[nodiscard]] VkDeviceSize getStorageBufferAlignment(VkDeviceSize size) const;

        // =---- Actions

        void clearQueues();

        // Window * window;
        //std::unique_ptr<Instance> instance;
        //std::shared_ptr<PhysicalDevice> physicalDevice;

        VmaAllocator allocator{};

        std::shared_ptr<CommandPool> transferCommandPool_;
        std::shared_ptr<Queue> graphicsQueue_;
        std::shared_ptr<Queue> transferQueue_;
        std::shared_ptr<Queue> computeQueue_;
        VkQueue presentQueue;
        
        //void createDevice();

        std::tuple<VkImageView, VkSemaphore, uint32_t>  acquireImage();
        void presentImage(VkImageView imageView, VkSemaphore readySemaphore);

        bool checkSwapChain();
        void replaceSwapChain();
        VkExtent2D getSwapChainExtent() const;
        uint32_t getSwapChainImageCount() const;
        VkFormat getSwapChainFormat() const;

    private:
        vkb::Instance instance;
        vkb::Device vkb_device;
        vkb::PhysicalDevice phys_device;

        vkb::Swapchain swapChain;
        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;
        std::vector<VkSemaphore> swapChainSemaphores;
        uint32_t swapChainIndex = 0;

        bool swapChainOutofDate = false;

        //VkPhysicalDeviceLimits phys_limits;
        VkDevice handle_;
        //std::shared_ptr<Surface> surface;
        VkSurfaceKHR surface_;
        std::unordered_map<RxUtil::Hash, VkSampler> samplers_;
        std::unordered_map<RxUtil::Hash, VkPipelineLayout> pipelineLayouts_;
        std::unordered_map<RxUtil::Hash, VkDescriptorSetLayout> descriptorSetLayouts_;

        std::optional<uint32_t> presentQueueFamily_{};
        //bool exclusiveQueueSupport_{};

        //VkSurfaceCapabilitiesKHR capabilities_{};
        //std::vector<VkSurfaceFormatKHR> formats_{};
        //std::vector<VkPresentModeKHR> presentationModes_{};
        //VkFormat selectedFormat_{};
        //VkColorSpaceKHR selectedColorSpace_{};
        //VkPresentModeKHR selectedPresentationMode_{};

        RxUtil::Hash getHashForSampler(VkSampler) const;
        RxUtil::Hash getHashForDescriptorSetLayout(VkDescriptorSetLayout) const;
        //std::unordered_map<RXUtil::Hash, std::shared_ptr<Sampler>> samplers_;
        RxUtil::Hash getHash(const VkSamplerCreateInfo & sci) const;
        RxUtil::Hash getHash(const VkPipelineLayoutCreateInfo & plci) const;
        RxUtil::Hash getHash(const VkDescriptorSetLayoutCreateInfo & dslci) const;
        RxUtil::Hash getHash(const VkDescriptorSetLayoutBindingFlagsCreateInfo & dslbfci) const;
        RxUtil::Hash getPNextHash(const void * pNext) const;

        //void createQueues();
    };

#if 0
    inline Device * iVulkan()
    {
        return Device::Context();
    }
#endif
} // namespace RXCore
