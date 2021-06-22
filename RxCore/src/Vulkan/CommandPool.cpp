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

//
// Created by shane on 29/12/2020.
//

#include "CommandPool.hpp"
#include "Device.h"
#include "CommandBuffer.hpp"
//#include "Renderer.hpp"

namespace RxCore
{
    CommandPool::CommandPool(Device * device, VkCommandPool handle, uint32_t qf)
        : handle(handle)
        , device_(device)
    //, queuefamily_(qf)
    {}

    CommandPool::~CommandPool()
    {
        device_->destroyCommandPool(handle);
        //vkDestroyCommandPool(device_->getDevice(), handle, nullptr);
    }

    std::shared_ptr<PrimaryCommandBuffer> CommandPool::GetPrimaryCommandBuffer()
    {
        VkCommandBufferAllocateInfo cbai{};
        cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cbai.commandPool = handle;
        cbai.commandBufferCount = 1;
        cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        std::vector<VkCommandBuffer> cb(1);

        auto res = vkAllocateCommandBuffers(device_->getDevice(), &cbai, cb.data());
        if (res != VK_SUCCESS) {
            throw std::runtime_error("Unable to allocate command buffer");
        }
        //auto res = device_->getDevice().allocateCommandBuffers(cbai);
        return std::make_shared<PrimaryCommandBuffer>(device_, cb[0], shared_from_this());
    }

    std::shared_ptr<SecondaryCommandBuffer> CommandPool::GetSecondaryCommandBuffer()
    {
        VkCommandBufferAllocateInfo cbai{};
        cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cbai.commandPool = handle;
        cbai.commandBufferCount = 1;
        cbai.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;

        std::vector<VkCommandBuffer> cb(1);
        auto res = vkAllocateCommandBuffers(device_->getDevice(), &cbai, cb.data());
        if (res != VK_SUCCESS) {
            throw std::runtime_error("Unable to allocate command buffer");
        }
        return std::make_shared<SecondaryCommandBuffer>(device_, cb[0], shared_from_this());
    }

    std::shared_ptr<TransferCommandBuffer> CommandPool::createTransferCommandBuffer()
    {
        VkCommandBufferAllocateInfo cbai{};
        cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cbai.commandPool = handle;
        cbai.commandBufferCount = 1;
        cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        std::vector<VkCommandBuffer> cb(1);

        auto res = vkAllocateCommandBuffers(device_->getDevice(), &cbai, cb.data());
        if (res != VK_SUCCESS) {
            throw std::runtime_error("Unable to allocate command buffer");
        }
        return std::make_shared<TransferCommandBuffer>(device_, cb[0], shared_from_this());
    }
} // namespace RXCore
