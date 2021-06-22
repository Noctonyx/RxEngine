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

#include "DescriptorPool.hpp"
#include "DescriptorSet.hpp"
#include "Log.h"
#include "optick/optick.h"

namespace RxCore
{
    DescriptorPool::DescriptorPool(Device * device, VkDescriptorPool new_handle)
        : handle(new_handle)
        , device_(device) { }

    std::shared_ptr<DescriptorSet> DescriptorPool::allocateDescriptorSet(
        VkDescriptorSetLayout layout)
    {
        OPTICK_EVENT()
        std::vector<VkDescriptorSetLayout> vv(1, VkDescriptorSetLayout(layout));

        VkDescriptorSetAllocateInfo dsai{};
        dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dsai.descriptorPool = handle;
        dsai.descriptorSetCount = 1;
        dsai.pSetLayouts = vv.data();

        std::vector<VkDescriptorSet> ds(1);

        auto res = vkAllocateDescriptorSets(device_->getDevice(), &dsai, ds.data());

        if (res != VK_SUCCESS) {
            return nullptr;
            //spdlog::critical("Unable to allocate DescriptorSet");
            //throw std::exception("Unable to allocate DescriptorSet");
        }
        return std::make_shared<DescriptorSet>(device_, shared_from_this(), ds[0]);
    }

    void DescriptorPool::handBackDescriptorSet(VkDescriptorSet descriptor_set)
    {
        OPTICK_EVENT()
        vkFreeDescriptorSets(device_->getDevice(), handle, 1, &descriptor_set);
    }

    std::shared_ptr<DescriptorSet> DescriptorPool::allocateDescriptorSet(
        VkDescriptorSetLayout layout,
        const std::vector<uint32_t> & counts)
    {
        OPTICK_EVENT()
        std::vector<VkDescriptorSetLayout> vv(1, VkDescriptorSetLayout(layout));

        VkDescriptorSetVariableDescriptorCountAllocateInfo dsvdcai{};
        dsvdcai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
        dsvdcai.descriptorSetCount = static_cast<uint32_t>(counts.size());
        dsvdcai.pDescriptorCounts = counts.data();

        VkDescriptorSetAllocateInfo dsai{};
        dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dsai.descriptorPool = handle;
        dsai.descriptorSetCount = 1;
        dsai.pSetLayouts = vv.data();
        dsai.pNext = &dsvdcai;

        std::vector<VkDescriptorSet> ds(1);

        auto res = vkAllocateDescriptorSets(device_->getDevice(), &dsai, ds.data());
        if (res != VK_SUCCESS) {
            return nullptr;
            //spdlog::critical("Unable to allocate DescriptorSet");
            //throw std::exception("Unable to allocate DescriptorSet");
        }
        return std::make_shared<DescriptorSet>(device_, shared_from_this(), ds[0]);
    }
#if 0
    DescriptorPoolGroup::DescriptorPoolGroup(const DescriptorPoolTemplate & poolTemplate)
        : poolSizes(poolTemplate.poolSizes)
        , max(poolTemplate.max)
    {
        OPTICK_EVENT()
    }

    std::shared_ptr<DescriptorSet> DescriptorPoolGroup::getDescriptorSet(VkDescriptorSetLayout layout)
    {
        OPTICK_EVENT();

        std::stringstream ss;
        ss << std::this_thread::get_id();

        if (!descriptorPool) {
            descriptorPool = iVulkan()->CreateDescriptorPool(poolSizes, max);

            spdlog::debug("Thread {} allocating a descriptorsetpool {}", ss.str(), descriptorPool->handle);
        }
        std::shared_ptr<DescriptorSet> ds;

        try {
            ds = descriptorPool->allocateDescriptorSet(layout);
        } catch (VkOutOfPoolMemoryError  ) {
            ds = nullptr;
        }
        if (ds) {
            spdlog::debug("Thread {} allocating a descriptorset {} ", ss.str(), ds->handle);

            return ds;
        }
        descriptorPool = iVulkan()->CreateDescriptorPool(poolSizes, max);
        spdlog::debug("*Thread {} allocating a descriptorsetpool {}", ss.str(), descriptorPool->handle);

        ds = descriptorPool->allocateDescriptorSet(layout);
        spdlog::debug("*Thread {} allocating a descriptorset {} ", ss.str(), ds->handle);
        return ds;
    }

    std::shared_ptr<DescriptorSet> DescriptorPoolGroup::getDescriptorSet(
        VkDescriptorSetLayout layout,
        const std::vector<uint32_t> & counts)
    {
        OPTICK_EVENT()
        std::stringstream ss;
        ss << std::this_thread::get_id();
        if (!descriptorPool) {
            descriptorPool = iVulkan()->CreateDescriptorPool(poolSizes, max);
            spdlog::debug("Thread {} allocating a descriptorsetpool {}", ss.str(), descriptorPool->handle);
        }
        auto ds = descriptorPool->allocateDescriptorSet(layout, counts);
        if (ds) {
            spdlog::debug("Thread {} allocating a descriptorset {}  with counts", ss.str(), ds->handle);
            return ds;
        }
        descriptorPool = iVulkan()->CreateDescriptorPool(poolSizes, max);
        spdlog::debug("*Thread {} allocating a descriptorsetpool {}", ss.str(), descriptorPool->handle);
        ds = descriptorPool->allocateDescriptorSet(layout, counts);
        spdlog::debug("*Thread {} allocating a descriptorset {}  with counts", ss.str(), ds->handle);
        return ds;
    }

    DescriptorPoolGroup::~DescriptorPoolGroup()
    {
        descriptorPool.reset();
    }
#endif
}
