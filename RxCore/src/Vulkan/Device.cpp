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

#include "Device.h"
#include "CommandPool.hpp"
#include "CommandBuffer.hpp"
#include "Buffer.hpp"
#include "Queue.hpp"
#include "Image.hpp"
#include "DescriptorPool.hpp"
#include "IndexBuffer.hpp"
#include "VertexBuffer.h"
#include "Shader.h"
#include <Hasher.h>
#include "Log.h"
#include "optick/optick.h"
#include "SDL_vulkan.h"

namespace RxCore
{
    VkBool32 debugMessage(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
        void * pUserData)
    {
        switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            spdlog::warn(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            spdlog::critical(pCallbackData->pMessage);
            //                throw std::exception(callbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            spdlog::info(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        default:
            spdlog::debug(pCallbackData->pMessage);
            break;
        }
        return VK_FALSE;
    }

    Device::Device(SDL_Window * window)
    {
        auto si_ret = vkb::SystemInfo::get_system_info();
        auto si = si_ret.value();

        uint32_t count;
        std::vector<const char *> names;

        SDL_Vulkan_GetInstanceExtensions(window, &count, nullptr);
        names.resize(count);
        SDL_Vulkan_GetInstanceExtensions(window, &count, names.data());

        vkb::InstanceBuilder instance_builder;

        auto ib = instance_builder
            .enable_validation_layers(true);

        for (auto n: names) {
            ib.enable_extension(n);
        }
        ib.require_api_version(1, 2, 0)
          .set_app_name("Rx")
          .set_engine_name("RxCore")
          .set_debug_callback(debugMessage);

        auto instance_return = ib.build();

        if (!instance_return) {
            spdlog::critical("Failed to created the instance!");
            throw std::runtime_error("Failed to created the instance!");
        }

        instance = instance_return.value();

        VkSurfaceKHR surface_khr = nullptr;
        if (!SDL_Vulkan_CreateSurface(window, instance.instance, &surface_khr)) {
            spdlog::critical("Failed to created the window surface!");
        }

        vkb::PhysicalDeviceSelector pds(instance);

        VkPhysicalDeviceFeatures feat{};
        feat.depthClamp = true;
        feat.multiDrawIndirect = true;
        feat.samplerAnisotropy = true;
        feat.sampleRateShading = true;

        VkPhysicalDeviceVulkan12Features feat12{};
        feat12.bufferDeviceAddress = true;
        feat12.descriptorBindingPartiallyBound = true;
        feat12.descriptorIndexing = true;
        feat12.shaderSampledImageArrayNonUniformIndexing = true;
        feat12.shaderStorageBufferArrayNonUniformIndexing = true;
        feat12.shaderUniformBufferArrayNonUniformIndexing = true;
        feat12.descriptorBindingVariableDescriptorCount = true;
        feat12.runtimeDescriptorArray = true;
        feat12.descriptorBindingPartiallyBound = true;
        feat12.descriptorBindingSampledImageUpdateAfterBind = true;

        auto pds_builder = pds.set_surface(surface_khr)
                              .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
                              .require_present(true)
                              .set_required_features(feat)
                              .set_required_features_12(feat12)
                              .add_required_extension(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME)
                              .add_required_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
                              .set_minimum_version(1, 2);

        auto pds_return = pds_builder.select();

        if (!pds_return) {
            spdlog::critical("Failed to created the physical device!");
            throw std::runtime_error("Failed to created the physical device!");
        }

        phys_device = pds_return.value();

        vkb::DeviceBuilder db(phys_device);

        auto dev_ret = db.build();
        if (!dev_ret) {
            spdlog::critical("Failed to created the device!");
            throw std::runtime_error("Failed to created the device!");
        }

        vkb_device = dev_ret.value();
        handle_ = vkb_device.device;

        VmaAllocatorCreateInfo allocator_info = {};
        allocator_info.vulkanApiVersion = VK_API_VERSION_1_2;
        allocator_info.physicalDevice = phys_device.physical_device;
        allocator_info.device = handle_;
        allocator_info.instance = instance.instance;
        allocator_info.flags =
            VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT |
            VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

#if _DEBUG
        VmaRecordSettings rs;
        rs.flags = 0;
        rs.pFilePath = "D:\\vma.record.csv";
#endif
        vmaCreateAllocator(&allocator_info, &allocator);

        VmaBudget budget[VK_MAX_MEMORY_HEAPS];
        vmaGetBudget(allocator, budget);
        surface_ = surface_khr;

        graphicsQueue_ = std::make_shared<Queue>(
            this,
            vkb_device.get_queue(vkb::QueueType::graphics).value(),
            vkb_device.get_queue_index(vkb::QueueType::graphics).value());

        transferQueue_ = std::make_shared<Queue>(
            this,
            vkb_device.get_queue(vkb::QueueType::transfer).value(),
            vkb_device.get_queue_index(vkb::QueueType::transfer).value());

        presentQueue = vkb_device.get_queue(vkb::QueueType::present).value();

        transferCommandPool_ = createCommandPool(
            vkb_device.get_queue_index(vkb::QueueType::transfer).value());

        vkb::SwapchainBuilder swapchain_builder{vkb_device};
        auto swap_ret = swapchain_builder
            .set_desired_format(
                {
                    VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
                }
            )
            .build();

        swapChain = swap_ret.value();
        auto r = swapChain.get_image_views();
        swapChainImageViews = r.value();
        swapChainImages = swapChain.get_images().value();
        swapChainSemaphores.resize(swapChainImageViews.size());
        for (auto & s: swapChainSemaphores) {
            s = createSemaphore();
        }

#if USE_OPTICK
        ::VkDevice d = handle_;
        ::VkPhysicalDevice pd = phys_device.physical_device;
        ::VkQueue q = graphicsQueue_->GetHandle();
        uint32_t qf = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
#endif
        OPTICK_GPU_INIT_VULKAN(&(d), &(pd), &(q), &(qf), 1, nullptr)
    }

    Device::~Device()
    {
        OPTICK_SHUTDOWN()

        clearQueues();

        for (auto & s: samplers_) {
            vkDestroySampler(handle_, s.second, nullptr);
        }

        for (auto & s: pipelineLayouts_) {
            vkDestroyPipelineLayout(handle_, s.second, nullptr);
        }

        for (auto & s: descriptorSetLayouts_) {
            vkDestroyDescriptorSetLayout(handle_, s.second, nullptr);
        }

        samplers_.clear();
        pipelineLayouts_.clear();
        descriptorSetLayouts_.clear();

        swapChain.destroy_image_views(swapChainImageViews);

        for (auto s: swapChainSemaphores) {
            destroySemaphore(s);
        }

        vkb::destroy_swapchain(swapChain);
        vkDestroySurfaceKHR(instance.instance, surface_, nullptr);

#if 1
        vmaDestroyAllocator(allocator);
        allocator = nullptr;
#endif
        vkDeviceWaitIdle(handle_);
        vkb::destroy_device(vkb_device);
        vkb::destroy_instance(instance);
    }

    void Device::clearQueues()
    {
        graphicsQueue_.reset();
        computeQueue_.reset();
        transferQueue_.reset();
        transferCommandPool_.reset();
    }

    std::tuple<VkImageView, VkSemaphore, uint32_t> Device::acquireImage()
    {
        std::tuple<VkImageView, VkSemaphore, uint32_t> result;
        auto sem = swapChainSemaphores[swapChainIndex];

        auto r = vkAcquireNextImageKHR(
            handle_, swapChain.swapchain,
            std::numeric_limits<uint64_t>::max(),
            sem,
            nullptr,
            &swapChainIndex
        );
        std::get<2>(result) = swapChainIndex;
        std::get<1>(result) = sem;

        if (r == VK_SUBOPTIMAL_KHR) {
            spdlog::info("Swapchain out of date");
            swapChainOutofDate = true;
        }
        if (r == VK_ERROR_OUT_OF_DATE_KHR) {
            spdlog::info("Swapchain out of date");
            swapChainOutofDate = true;
            std::get<0>(result) = nullptr;
            return result;
        }
        std::get<0>(result) = swapChainImageViews[swapChainIndex];
        return result;
    }

    void Device::presentImage(VkImageView imageView, VkSemaphore readySemaphore)
    {
        VkPresentInfoKHR pi{};
        pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        pi.waitSemaphoreCount = 1;
        pi.pWaitSemaphores = &readySemaphore;
        pi.swapchainCount = 1;
        pi.pSwapchains = &swapChain.swapchain;
        pi.pImageIndices = &swapChainIndex;

        auto r = vkQueuePresentKHR(presentQueue, &pi);

        if (r == VK_ERROR_OUT_OF_DATE_KHR) {
            swapChainOutofDate = true;
            return;
        }
        assert(r == VK_SUCCESS);

        if (r != VK_SUCCESS) {
            throw std::exception("Unable to present");
        }
    }

    bool Device::checkSwapChain()
    {
        if (swapChainOutofDate) {
            replaceSwapChain();
            swapChainOutofDate = false;
            return true;
        }
        return false;
    }

    void Device::replaceSwapChain()
    {
        WaitIdle();

        swapChain.destroy_image_views(swapChainImageViews);

        vkb::SwapchainBuilder scb{vkb_device};
        scb.set_old_swapchain(swapChain);
        auto sb = scb
            .set_desired_format(
                {
                    VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
                }
            )
            .build();
        vkb::destroy_swapchain(swapChain);

        swapChain = sb.value();

        auto r = swapChain.get_image_views();
        swapChainImageViews = r.value();
        swapChainImages = swapChain.get_images().value();
        if (swapChainImageViews.size() != swapChainSemaphores.size()) {
            for (auto s: swapChainSemaphores) {
                destroySemaphore(s);
            }
            swapChainSemaphores.resize(swapChainImageViews.size());
            for (auto & s: swapChainSemaphores) {
                s = createSemaphore();
            }
        }
    }

    VkExtent2D Device::getSwapChainExtent() const
    {
        return swapChain.extent;
    }

    uint32_t Device::getSwapChainImageCount() const
    {
        return static_cast<uint32_t>(swapChainImages.size());
    }

    VkFormat Device::getSwapChainFormat() const
    {
        return swapChain.image_format;
    }

    void Device::transferBuffer(
        std::shared_ptr<Buffer> src,
        std::shared_ptr<Buffer> dst,
        size_t size,
        size_t srcOffset,
        size_t destOffset) const
    {
        auto cb = transferCommandPool_->createTransferCommandBuffer();

        cb->begin();
        cb->copyBuffer(std::move(src), std::move(dst), srcOffset, destOffset, size);
        cb->end();
        cb->submitAndWait();
    }

    void Device::transitionImageLayout(
        const std::shared_ptr<Image> & image,
        VkImageLayout dstLayout) const
    {
        auto cb = transferCommandPool_->createTransferCommandBuffer();

        cb->begin();

        VkPipelineStageFlags src_stage, dest_stage;

        VkImageMemoryBarrier imb{};
        imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imb.image = image->handle();
        imb.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imb.subresourceRange.baseMipLevel = 0;
        imb.subresourceRange.levelCount = 1;
        imb.subresourceRange.baseArrayLayer = 0;
        imb.subresourceRange.layerCount = 1;
        imb.oldLayout = image->currentLayout_;
        imb.newLayout = dstLayout;
        imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        if (image->currentLayout_ == VK_IMAGE_LAYOUT_UNDEFINED && dstLayout ==
                                                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            imb.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (image->currentLayout_ == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                   dstLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            imb.dstAccessMask = (VK_ACCESS_TRANSFER_WRITE_BIT);
            imb.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }
        vkCmdPipelineBarrier(
            cb->Handle(), src_stage, dest_stage, {}, 0, nullptr, 0, nullptr, 1,
            &imb
        );

        cb->end();
        cb->submitAndWait();

        image->currentLayout_ = dstLayout;
    }

    void Device::transferBufferToImage(
        std::shared_ptr<Buffer> src,
        std::shared_ptr<Image> dst,
        VkExtent3D extent,
        VkImageLayout destLayout,
        uint32_t layerCount,
        uint32_t baseArrayLayer,
        uint32_t mipLevel) const
    {
        assert(src);
        assert(dst);

        auto cb = transferCommandPool_->createTransferCommandBuffer();

        cb->begin();

        // if (dst->currentLayout_ != VkImageLayout::eTransferDstOptimal) {
        cb->imageTransition(dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevel);
        dst->currentLayout_ = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        // TransitionImageLayout(dst, VkImageLayout::eTransferDstOptimal);
        //  }

        cb->copyBufferToImage(
            src, dst, extent, layerCount, baseArrayLayer,
            mipLevel
        );
        cb->imageTransition(dst, destLayout, mipLevel);
        cb->end();
        cb->submitAndWait();
    }

    VkDeviceSize Device::getUniformBufferAlignment(VkDeviceSize size) const
    {
        if (size % phys_device.properties.limits.minUniformBufferOffsetAlignment == 0) {
            return size;
        }
        return ((size / phys_device.properties.limits.minUniformBufferOffsetAlignment) + 1) *
               phys_device.properties.limits.minUniformBufferOffsetAlignment;
    }

    VkDeviceSize Device::getStorageBufferAlignment(VkDeviceSize size) const
    {
        if (size % phys_device.properties.limits.minStorageBufferOffsetAlignment == 0) {
            return size;
        }
        return ((size / phys_device.properties.limits.minStorageBufferOffsetAlignment) + 1) *
               phys_device.properties.limits.minStorageBufferOffsetAlignment;
    }

    std::shared_ptr<Buffer> Device::createBuffer(
        const VkBufferUsageFlags & flags,
        VmaMemoryUsage memType,
        const VkDeviceSize size,
        void * data)
    {
        OPTICK_EVENT()

        VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        buffer_info.size = size;
        buffer_info.usage = static_cast<VkBufferUsageFlags>(flags);
        if (data) {
            buffer_info.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }

        VmaAllocationCreateInfo allocation_create_info = {};
        allocation_create_info.usage = memType;

        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo alloc_info;
        vmaCreateBuffer(
            allocator, &buffer_info, &allocation_create_info, &buffer, &allocation,
            &alloc_info
        );

        //assert(res == VK_SUCCESS );

        auto alloc = std::make_shared<Allocation>(allocator, allocation);
        auto mb = std::make_shared<Buffer>(this, buffer, alloc, size);

        if (data) {
            VkMemoryPropertyFlags memFlags;
            vmaGetMemoryTypeProperties(allocator, alloc_info.memoryType, &memFlags);
            if ((memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
                // Allocation ended up in mappable memory. You can map it and access it directly.
                void * mapped_data;

                vmaMapMemory(allocator, allocation, &mapped_data);
                memcpy(mapped_data, data, size);
                vmaUnmapMemory(allocator, allocation);
            } else {
                auto stage_buf = createStagingBuffer(size, data);
                transferBuffer(stage_buf, mb, size);
                // Allocation ended up in non-mappable memory.
                // You need to create CPU-side buffer in VMA_MEMORY_USAGE_CPU_ONLY and make a
                // transfer.
            }
        }

        return mb;
    }

    std::shared_ptr<VertexBuffer> Device::createVertexBuffer(
        VmaMemoryUsage memType,
        uint32_t vertexCount,
        uint32_t vertexSize,
        void * data)
    {
        auto s = vertexCount * vertexSize;

        VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        buffer_info.size = s;
        buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        if (data) {
            buffer_info.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }

        VmaAllocationCreateInfo alloc_info = {};
        alloc_info.usage = memType;

        VkBuffer buffer;
        VmaAllocation allocation;
        vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &buffer, &allocation, nullptr);
        auto alloc = std::make_shared<Allocation>(allocator, allocation);

        auto mb = std::make_shared<VertexBuffer>(this, buffer, alloc, vertexCount, vertexSize);
        if (data) {
            auto stage_buf = createStagingBuffer(s, data);
            transferBuffer(stage_buf, mb, s);
        }

        return mb;
    }

    std::shared_ptr<IndexBuffer> Device::createIndexBuffer(
        VmaMemoryUsage memType,
        uint32_t indexCount,
        bool is16,
        void * data)
    {
        auto s = (is16 ? sizeof(uint16_t) : sizeof(uint32_t)) * indexCount;

        VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        buffer_info.size = s;
        buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        if (data) {
            buffer_info.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }

        VmaAllocationCreateInfo alloc_info = {};
        alloc_info.usage = memType;

        VkBuffer buffer;
        VmaAllocation allocation;
        vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &buffer, &allocation, nullptr);
        auto alloc = std::make_shared<Allocation>(allocator, allocation);
        auto mb = std::make_shared<IndexBuffer>(this, buffer, alloc, indexCount, is16);

        if (data) {
            auto buf = createStagingBuffer(s, data);
            transferBuffer(buf, mb, s);
        }
        return mb;
    }

    std::shared_ptr<Image> Device::createImage(
        const VkFormat format,
        const VkExtent3D extent,
        uint32_t mipLevels,
        uint32_t layers,
        VkImageUsageFlags usage,
        VkImageType type)
    {
        VkImageCreateInfo ici{};
        ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ici.imageType = type;
        ici.format = format;
        ici.extent = extent;
        ici.mipLevels = mipLevels;
        ici.arrayLayers = layers;
        ici.samples = VK_SAMPLE_COUNT_1_BIT;
        ici.tiling = VK_IMAGE_TILING_OPTIMAL;
        ici.usage = usage;
        ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkImage image;
        VmaAllocation allocation;
        VmaAllocationCreateInfo alloc_info = {};
        alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        vmaCreateImage(
            allocator, &ici, &alloc_info, &image,
            &allocation, nullptr
        );

        auto i = std::make_shared<Image>(
            this, image, format, std::make_shared<Allocation>(allocator, allocation), extent
        );

        return i;
    }

    std::shared_ptr<Shader> Device::createShader(const std::vector<uint32_t> & bytes) const
    {
        VkShaderModuleCreateInfo smci{};
        smci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        smci.codeSize = static_cast<uint32_t>(bytes.size() * 4);
        smci.pCode = bytes.data();

        VkShaderModule sm;
        vkCreateShaderModule(handle_, &smci, nullptr, &sm);
        return std::make_shared<Shader>(this, sm);
    }

    std::shared_ptr<DescriptorPool> Device::CreateDescriptorPool(
        std::vector<VkDescriptorPoolSize> poolSizes,
        uint32_t max)
    {
        VkDescriptorPoolCreateInfo dpci{};
        dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        dpci.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        dpci.pPoolSizes = poolSizes.data();
        dpci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT |
                     VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        dpci.maxSets = max;

        VkDescriptorPool dp;
        vkCreateDescriptorPool(handle_, &dpci, nullptr, &dp);

        return std::make_shared<DescriptorPool>(this, dp);
    }

    void Device::WaitIdle() const
    {
        vkDeviceWaitIdle(handle_);
    }

    std::shared_ptr<CommandPool> Device::CreateGraphicsCommandPool()
    {
        auto ix = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

        return createCommandPool(ix);
    }

    VkFormat Device::GetDepthFormat(bool checkSamplingSupport) const
    {
        std::vector<VkFormat> formats = {
            VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM,
        };

        for (auto & format: formats) {
            VkFormatProperties format_properties;

            vkGetPhysicalDeviceFormatProperties(
                phys_device.physical_device, format,
                &format_properties
            );

            if (format_properties.optimalTilingFeatures &
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                if (checkSamplingSupport) {
                    if (!(format_properties.optimalTilingFeatures &
                          VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
                        continue;
                    }
                }
                return format;
            }
        }
        spdlog::critical("Failed to find suitable depth format ");
        return VK_FORMAT_UNDEFINED;
    }

    void Device::getMemBudget(std::vector<MemHeapStatus> & heaps) const
    {
        VkPhysicalDeviceMemoryProperties2 pdmp2{};
        pdmp2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;

        VkPhysicalDeviceMemoryBudgetPropertiesEXT ex{};
        ex.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;

        pdmp2.pNext = &ex;

        vkGetPhysicalDeviceMemoryProperties2(phys_device.physical_device, &pdmp2);

        heaps.clear();

        uint32_t hc = pdmp2.memoryProperties.memoryHeapCount;
        for (uint32_t i = 0; i < hc; i++) {
            (void) heaps.emplace_back(MemHeapStatus{ex.heapBudget[i], ex.heapUsage[i]});
        }
    }

    std::shared_ptr<Buffer> Device::createStagingBuffer(size_t size, const void * data)
    {
        VkBufferCreateInfo buffer_info{
            VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, nullptr, 0, size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        };

        VmaAllocationCreateInfo alloc_info{0, VMA_MEMORY_USAGE_CPU_ONLY};

        VkBuffer buffer_handle;
        VmaAllocation allocation;

        vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &buffer_handle, &allocation, nullptr);

        auto all = std::make_shared<Allocation>(allocator, allocation);
        auto buf = std::make_shared<Buffer>(this, buffer_handle, all, size);
        all->map();
        all->update(data, size);
        all->unmap();

        return buf;
    }

    RxUtil::Hash Device::getHash(const VkSamplerCreateInfo & sci) const
    {
        RxUtil::Hasher h;

        h.f32(sci.maxAnisotropy);
        h.f32(sci.mipLodBias);
        h.f32(sci.minLod);
        h.f32(sci.maxLod);
        h.u32(static_cast<uint32_t>(sci.minFilter));
        h.u32(static_cast<uint32_t>(sci.magFilter));
        h.u32(static_cast<uint32_t>(sci.mipmapMode));
        h.u32(sci.compareEnable);
        h.u32(static_cast<uint32_t>(sci.compareOp));
        h.u32(sci.anisotropyEnable);
        h.u32(static_cast<uint32_t>(sci.addressModeU));
        h.u32(static_cast<uint32_t>(sci.addressModeV));
        h.u32(static_cast<uint32_t>(sci.addressModeW));
        h.u32(static_cast<uint32_t>(sci.borderColor));
        h.u32(sci.unnormalizedCoordinates);

        return h.get();
    }

    VkSampler Device::createSampler(const VkSamplerCreateInfo & sci)
    {
        auto h = getHash(sci);

        if (samplers_.contains(h)) {
            return samplers_[h];
        }

        VkSampler samp;
        vkCreateSampler(handle_, &sci, nullptr, &samp);
        samplers_.emplace(h, samp);

        return samp;
    }

    std::shared_ptr<CommandPool> Device::createCommandPool(uint32_t index)
    {
        VkCommandPoolCreateInfo cci{};
        cci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cci.queueFamilyIndex = index;
        VkCommandPool cp;
        vkCreateCommandPool(handle_, &cci, nullptr, &cp);
        //auto h = handle_.createCommandPool(cci);
        return std::make_shared<CommandPool>(this, cp, cci.queueFamilyIndex);
    }

    VkPipelineLayout Device::createPipelineLayout(const VkPipelineLayoutCreateInfo & plci)
    {
        auto h = getHash(plci);
        if (pipelineLayouts_.contains(h)) {
            return pipelineLayouts_[h];
        }

        std::vector<VkDescriptorSetLayout> dsls(plci.setLayoutCount);
        //std::copy(plci.pSetLayouts, plci.pSetLayouts + (plci.setLayoutCount-1), dsls);

        VkPipelineLayout layout;
        vkCreatePipelineLayout(handle_, &plci, nullptr, &layout);
        //std::make_shared<RXCore::PipelineLayout>(handle_, );
        pipelineLayouts_.emplace(h, layout);

        return layout;
    }

    VkDescriptorSetLayout Device::createDescriptorSetLayout(
        const VkDescriptorSetLayoutCreateInfo & dslci)
    {
        auto h = getHash(dslci);
        if (descriptorSetLayouts_.contains(h)) {
            return descriptorSetLayouts_[h];
        }

        VkDescriptorSetLayout layout;
        vkCreateDescriptorSetLayout(handle_, &dslci, nullptr, &layout);

        descriptorSetLayouts_.emplace(h, layout);

        return layout;
    }

    RxUtil::Hash Device::getHash(const VkDescriptorSetLayoutCreateInfo & dslci) const
    {
        RxUtil::Hasher h;

        h.u32(dslci.bindingCount);
        h.u32(static_cast<uint32_t>(dslci.flags));
        for (uint32_t i = 0; i < dslci.bindingCount; i++) {
            auto & binding = dslci.pBindings[i];
            h.u32(binding.binding);
            h.u32(binding.descriptorCount);
            h.u32(static_cast<uint32_t>(binding.descriptorType));
            h.u32(static_cast<uint32_t>(binding.stageFlags));

            if (binding.pImmutableSamplers &&
                (binding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
                 binding.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER)) {
                for (uint32_t j = 0; j < binding.descriptorCount; j++) {
                    //RxUtil::Hasher hash;
                    auto hh = getHashForSampler(binding.pImmutableSamplers[j]);
                    h.u64(hh);
                }
            }
        }

        if (dslci.pNext) {
            auto h2 = getPNextHash(dslci.pNext);
            h.u64(h2);
        }
        return h.get();
    }

    RxUtil::Hash Device::getHashForSampler(VkSampler sampler) const
    {
        for (auto & s: samplers_) {
            if (s.second == sampler) {
                return s.first;
            }
        }
        assert(false);
        return 0;
    }

    RxUtil::Hash Device::getHashForDescriptorSetLayout(VkDescriptorSetLayout dsl) const
    {
        for (auto & s: descriptorSetLayouts_) {
            if (s.second == dsl) {
                return s.first;
            }
        }
        assert(false);
        return 0;
    }

    RxUtil::Hash Device::getHash(const VkPipelineLayoutCreateInfo & plci) const
    {
        RxUtil::Hasher h;

        h.u32(plci.setLayoutCount);
        for (uint32_t i = 0; i < plci.setLayoutCount; i++) {
            if (plci.pSetLayouts[i]) {
                RxUtil::Hash hash = getHashForDescriptorSetLayout(plci.pSetLayouts[i]);
                h.u64(hash);
            } else {
                h.u32(0);
            }
        }

        h.u32(plci.pushConstantRangeCount);
        for (uint32_t i = 0; i < plci.pushConstantRangeCount; i++) {
            auto & push = plci.pPushConstantRanges[i];
            h.u32(static_cast<uint32_t>(push.stageFlags));
            h.u32(push.size);
            h.u32(push.offset);
        }

        h.u32(static_cast<uint32_t>(plci.flags));

        return h.get();
    }

    RxUtil::Hash Device::getHash(
        const VkDescriptorSetLayoutBindingFlagsCreateInfo & dslbfci) const
    {
        RxUtil::Hasher h;
        h.u32(dslbfci.bindingCount);
        for (uint32_t i = 0; i < dslbfci.bindingCount; i++) {
            h.u32(static_cast<uint32_t>(dslbfci.pBindingFlags[i]));
        }
        return h.get();
    }

    RxUtil::Hash Device::getPNextHash(const void * pNext) const
    {
        RxUtil::Hasher h;
        auto v = reinterpret_cast<const VkBaseInStructure *>(pNext);

        h.u32(static_cast<uint32_t>(v->sType));
        RxUtil::Hash next_hash;

        switch (v->sType) {
        case VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO:
            next_hash = getHash(
                *static_cast<const VkDescriptorSetLayoutBindingFlagsCreateInfo *>(pNext));
            h.u64(next_hash);
            break;
        default:
            next_hash = 0;
            break;
        }

        return h.get();
    }

    void Device::destroyDescriptorPool(DescriptorPool * pool)
    {
        vkDestroyDescriptorPool(handle_, pool->handle, nullptr);
    }

    void Device::freeCommandBuffer(CommandBuffer * buf)
    {
        vkFreeCommandBuffers(handle_, buf->commandPool_->GetHandle(), 1, &buf->handle_);
    }

    uint32_t Device::getPresentQueueFamily() const
    {
        return presentQueueFamily_.value();
    }

    std::shared_ptr<Queue> Device::getTransferQueue() const
    {
        return transferQueue_;
    }

    VkSemaphore Device::createSemaphore() const
    {
        VkSemaphoreCreateInfo sci{};
        sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkSemaphore s;

        vkCreateSemaphore(handle_, &sci, nullptr, &s);

        return s;
    }

    void Device::destroySemaphore(VkSemaphore s) const
    {
        vkDestroySemaphore(handle_, s, nullptr);
    }

    VkFence Device::createFence() const
    {
        VkFenceCreateInfo fci{};
        fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        VkFence fence;
        vkCreateFence(handle_, &fci, nullptr, &fence);
        return fence;
    }

    void Device::destroyFence(VkFence f) const
    {
        vkDestroyFence(handle_, f, nullptr);
    }

    VkResult Device::waitForFence(VkFence f) const
    {
        return vkWaitForFences(handle_, 1, &f, true, MAXUINT64);
    }

    VkResult Device::getFenceStatus(VkFence f) const
    {
        return vkGetFenceStatus(handle_, f);
    }

    void Device::destroyImage(Image * image) const
    {
        vkDestroyImage(handle_, image->handle_, nullptr);
    }

    void Device::destroyImageView(ImageView * imageView) const
    {
        vkDestroyImageView(handle_, imageView->handle_, nullptr);
    }

    std::shared_ptr<ImageView> Device::createImageView(const std::shared_ptr<Image> & image,
                                                       VkImageViewType viewType,
                                                       VkImageAspectFlagBits aspect,
                                                       uint32_t baseArrayLayer,
                                                       uint32_t layerCount) const
    {
        VkImageViewCreateInfo ivci{};
        ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ivci.format = image->format_;
        ivci.viewType = viewType;
        ivci.image = image->handle_;
        ivci.subresourceRange.aspectMask = aspect;
        ivci.subresourceRange.baseMipLevel = 0;
        ivci.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        ivci.subresourceRange.baseArrayLayer = baseArrayLayer;
        ivci.subresourceRange.layerCount = layerCount;

        VkImageView iv;
        vkCreateImageView(handle_, &ivci, nullptr, &iv);

        return std::make_shared<ImageView>(this, iv);
    }

    uint64_t Device::getBufferAddress(const Buffer * buffer) const
    {
        VkBufferDeviceAddressInfo bdai{};
        bdai.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        bdai.buffer = buffer->handle_;

        return vkGetBufferDeviceAddress(handle_, &bdai);
    }

    void Device::destroyBuffer(VkBuffer buffer) const
    {
        vkDestroyBuffer(handle_, buffer, nullptr);
    }

    void Device::destroyCommandPool(VkCommandPool pool) const
    {
        vkDestroyCommandPool(handle_, pool, nullptr);
    }
} // namespace RXCore
