////////////////////////////////////////////////////////////////////////////////
// MIT License
//
// Copyright (c) 2020-2021.  Shane Hyde (shane@noctonyx.com)
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

#include "DescriptorSet.hpp"
#include <utility>
#include <algorithm>
#include <optional>
#include "Buffer.hpp"
#include "Image.hpp"
#include "DescriptorPool.hpp"

namespace RxCore
{
    DescriptorSet::DescriptorSet(
        Device * device,
        std::shared_ptr<DescriptorPool> descriptorPool,
        VkDescriptorSet newHandle
    )
    //: VulkanResource<DescriptorSet, VkDescriptorSet>(context)
        : handle(newHandle)
        , device_(device)
        , descriptorPool_(std::move(descriptorPool)) {}

    DescriptorSet::~DescriptorSet()
    {
        buffers_.clear();
        descriptorPool_->handBackDescriptorSet(handle);
    }

    void DescriptorSet::updateDescriptor(
        uint32_t binding,
        VkDescriptorType type,
        std::shared_ptr<Buffer> buffer,
        const uint32_t range,
        const uint32_t offset
    )
    {
        offsets_.resize(std::max(binding + 1, static_cast<uint32_t>(offsets_.size())));

        VkDescriptorBufferInfo dbi{buffer->handle(), 0, range == 0 ? buffer->size_ : range};

        std::vector<VkWriteDescriptorSet> wds(1);

        wds[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        wds[0].dstSet = handle;
        wds[0].dstBinding = binding;
        wds[0].descriptorCount = 1;
        wds[0].descriptorType = type;
        wds[0].pBufferInfo = &dbi;

        buffers_.insert_or_assign(binding, std::move(buffer));

        if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
            type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
            offsets_[binding] = offset;
        } else {
            offsets_[binding] = std::nullopt;
        }
        vkUpdateDescriptorSets(device_->getDevice(), static_cast<uint32_t>(wds.size()), wds.data(),
                               0, nullptr);
    }

    // TODO: fix this
    void DescriptorSet::updateDescriptor(
        uint32_t binding,
        VkDescriptorType type,
        const std::shared_ptr<Image> & image,
        VkSampler sampler
    )
    {
        offsets_.resize(std::max(binding + 1, static_cast<uint32_t>(offsets_.size())));

        auto image_view = device_->createImageView(
            image,
            VK_IMAGE_VIEW_TYPE_2D,
            VK_IMAGE_ASPECT_COLOR_BIT, 0, 1);

        if (imageViews_.contains(binding)) {
            imageViews_.erase(binding);
        }
        imageViews_.emplace(binding, std::vector<std::shared_ptr<ImageView>>{image_view});

        VkDescriptorImageInfo dii = {
            sampler, image_view->handle_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        VkWriteDescriptorSet wds{};
        wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        wds.descriptorType = type;
        wds.descriptorCount = 1;
        wds.dstArrayElement = 0;
        wds.dstSet = handle;
        wds.dstBinding = binding;
        wds.pImageInfo = &dii;

        vkUpdateDescriptorSets(device_->getDevice(), 1, &wds, 0, nullptr);
        //        device_->getDevice().updateDescriptorSets({wds}, {});
    }

    void DescriptorSet::updateDescriptor(
        uint32_t binding,
        VkDescriptorType type,
        std::shared_ptr<ImageView> imageView,
        VkImageLayout layout,
        VkSampler sampler)
    {
        offsets_.resize(std::max(binding + 1, static_cast<uint32_t>(offsets_.size())));
        VkDescriptorImageInfo dii = {
            sampler, imageView->handle_, layout
        };
        if (imageViews_.contains(binding)) {
            imageViews_.erase(binding);
        }
        imageViews_.emplace(binding, std::vector<std::shared_ptr<ImageView>>{imageView});

        VkWriteDescriptorSet wds;
        wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        wds.descriptorType = type;
        wds.descriptorCount = 1;
        wds.dstSet = handle;
        wds.dstBinding = binding;
        wds.pImageInfo = &dii;

        vkUpdateDescriptorSets(device_->getDevice(), 1, &wds, 0, nullptr);
    }
#if 0
    std::shared_ptr<ImageView> DescriptorSet::getBoundImageView(uint32_t binding) const
    {
        if (imageViews_.size() <= binding) {
            return nullptr;
        }
        return imageViews_[binding];
    }
#endif
    void DescriptorSet::freeResources()
    {
        buffers_.clear();
        //images_.clear();
        //samplers_.clear();
        imageViews_.clear();
    }

    void DescriptorSet::updateDescriptor(
        uint32_t binding,
        VkDescriptorType type,
        const std::vector<CombinedSampler> & samplerViews)
    {
        if (samplerViews.size() == 0) {
            return;
        }
        offsets_.resize(std::max(binding + 1, static_cast<uint32_t>(offsets_.size())));

        std::vector<VkDescriptorImageInfo> dii = {};
        for (const auto & sampler_view: samplerViews) {
            dii.push_back({
                sampler_view.sampler,
                sampler_view.imageView->handle_,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
        }
        std::vector<std::shared_ptr<ImageView>> iviews(samplerViews.size());
        std::transform(
            samplerViews.cbegin(), samplerViews.cend(),
            iviews.begin(),
            [](const CombinedSampler & i) -> std::shared_ptr<ImageView>
            {
                return i.imageView;
            });

        if (imageViews_.contains(binding)) {
            imageViews_.erase(binding);
        }
        imageViews_.emplace(binding, iviews);
        //std::vector<VkDescriptorImageInfo> dii;

        VkWriteDescriptorSet wds{};
        wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        wds.descriptorType = type;
        wds.descriptorCount = static_cast<uint32_t>(dii.size());
        wds.dstSet = handle;
        wds.dstBinding = binding;
        wds.pImageInfo = dii.data();

        vkUpdateDescriptorSets(device_->getDevice(), 1, &wds, 0, nullptr);
    }

    std::vector<uint32_t> DescriptorSet::getOffsets() const
    {
        std::vector<uint32_t> v;

        for (auto & x: offsets_) {
            if (x.has_value()) {
                v.push_back(x.value());
            }
        }
        return v;
    }
}
