//
// Copyright (c) 2020 - Shane Hyde (shane@noctonyx.com)
//

#pragma once

#include <vector>
#include <optional>
#include <unordered_map>
#include <memory>
#include "Vulk.hpp"
#include "Util.h"

namespace RxCore
{
    class Buffer;
    class Image;
    class ImageView;
    class DescriptorPool;
    class Device;

    struct CombinedSampler {
        VkSampler sampler;
        std::shared_ptr<ImageView> imageView;
    };

    class DescriptorSet
    {
    public:
        DescriptorSet(Device * device,
                      std::shared_ptr<DescriptorPool> descriptorPool,
                      VkDescriptorSet newHandle);

        RX_NO_COPY_NO_MOVE(DescriptorSet);

        ~DescriptorSet();

        void updateDescriptor(uint32_t binding,
                              VkDescriptorType type,
                              std::shared_ptr<Buffer> buffer,
                              const uint32_t range = 0,
                              const uint32_t offset = 0);

        void updateDescriptor(uint32_t binding,
                              VkDescriptorType type,
                              const std::shared_ptr<Image> & image,
                              VkSampler sampler);

        void updateDescriptor(
            uint32_t binding,
            VkDescriptorType type,
            const std::vector<CombinedSampler> &
                samplerViews);

        void updateDescriptor(uint32_t binding,
                              VkDescriptorType type,
                              std::shared_ptr<ImageView> imageView,
                              VkImageLayout layout,
                              VkSampler sampler);

        void setDescriptorOffset(uint32_t binding, const uint32_t offset) {
            offsets_[binding] = offset;
        }

        const VkDescriptorSet handle;

        void freeResources();

        std::vector<uint32_t> getOffsets() const;

    private:
        Device * device_;
        std::shared_ptr<DescriptorPool> descriptorPool_;
        std::unordered_map<uint32_t, std::shared_ptr<Buffer>> buffers_;
        std::unordered_map<uint32_t, std::vector<std::shared_ptr<ImageView>>> imageViews_;
        std::vector<std::optional<uint32_t>> offsets_;
    };
} // namespace RXCore
