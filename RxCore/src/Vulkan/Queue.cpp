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
// Created by shane on 28/12/2020.
//

#include <algorithm>
#include "Queue.hpp"
#include "Device.h"
#include "CommandBuffer.hpp"

namespace RxCore
{
    Queue::Queue(Device * device, const VkQueue queue, uint32_t family)
        : device_(device)
        , handle(queue)
        , family_(family) {}

    Queue::~Queue()
    {
        for (auto & [fence, data]: resources_) {
            device_->destroyFence(fence);
        }

        while (!resources_.empty()) {
            resources_.pop_front();
        }
    }

    void Queue::submitAndWait(std::shared_ptr<PrimaryCommandBuffer> & buffer) const
    {
        std::vector<VkCommandBuffer> buffer_handles = {buffer->Handle()};

        VkSubmitInfo si{};
        si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.commandBufferCount = static_cast<uint32_t>(buffer_handles.size());
        si.pCommandBuffers = buffer_handles.data();

        //        VkSubmitInfo si{nullptr, nullptr, buffer_handles};
        auto fence = device_->createFence();

        vkQueueSubmit(handle, 1, &si, fence);

        const auto result = device_->waitForFence(fence);
        assert(result == VK_SUCCESS);

        device_->destroyFence(fence);
    }

    void Queue::Submit(std::vector<std::shared_ptr<PrimaryCommandBuffer>> buffs,
                       std::vector<VkSemaphore> waitSems,
                       std::vector<VkPipelineStageFlags> waitStages,
                       std::vector<VkSemaphore> signalSemaphores)
    {
        auto fence = device_->createFence();

        std::vector<VkCommandBuffer> buffer_handles(buffs.size());

        std::ranges::transform(
            buffs, buffer_handles.begin(),
            [](std::shared_ptr<PrimaryCommandBuffer> & b)
            {
                return b->Handle();
            }
        );

        assert(waitStages.size() == waitSems.size());

        VkSubmitInfo si{};
        si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.waitSemaphoreCount = static_cast<uint32_t>(waitSems.size());
        si.pWaitSemaphores = waitSems.data();
        si.pWaitDstStageMask = waitStages.data();
        si.commandBufferCount = static_cast<uint32_t>(buffer_handles.size());
        si.pCommandBuffers = buffer_handles.data();
        si.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
        si.pSignalSemaphores = signalSemaphores.data();

        //VkSubmitInfo si{waitSems, waitStages, buffer_handles, signalSemaphores};

        resources_.emplace_back(fence, std::move(buffs));

        vkQueueSubmit(handle, 1, &si, fence);
    }

    void Queue::ReleaseCompleted()
    {
        while (!resources_.empty()) {
            auto & [fence, data] = resources_.front();

            auto fs = device_->getFenceStatus(fence);
            if (fs == VK_SUCCESS) {
                resources_.pop_front();
                device_->destroyFence(fence);
            } else {
                break;
            }
        }
    }

    VkQueue Queue::GetHandle() const
    {
        return handle;
    }
} // namespace RXCore
