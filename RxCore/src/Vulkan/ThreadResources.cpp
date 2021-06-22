//
// Created by shane on 14/02/2021.
//

#include "ThreadResources.h"
#include "Vulkan/Device.h"
#include "Vulkan/CommandPool.hpp"
#include "optick/optick.h"

namespace RxCore
{
    thread_local ThreadResources threadResources;

    void ThreadResources::freeAllResources()
    {
        pool.reset();
        buffers.clear();
    }

    std::shared_ptr<SecondaryCommandBuffer> ThreadResources::getCommandBuffer()
    {
        if (pool == nullptr) {
            pool = device->CreateGraphicsCommandPool();
        }

        auto b = pool->GetSecondaryCommandBuffer();
        buffers.push_back(b);
        return b;
    }

    void ThreadResources::freeUnused()
    {
        OPTICK_EVENT("Check for buffers to free")
        while (!buffers.empty() && buffers.front().use_count() == 1) {
            OPTICK_EVENT("Free buffer")
            buffers.pop_front();
        }
    }

    void ThreadResources::setDevice(RxCore::Device * d)
    {
        device = d;
    }
}