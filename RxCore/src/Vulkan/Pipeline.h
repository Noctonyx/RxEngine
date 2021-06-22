#pragma once

#include "Vulk.hpp"
#include "Util.h"
#include "Device.h"

namespace RxCore
{
    class Pipeline
    {
    public:
        explicit Pipeline(Device * device, const VkPipeline & handle)
            : device_(device)
            , handle_(handle)
        {}

        RX_NO_COPY_NO_MOVE(Pipeline);

        ~Pipeline()
        {
            vkDestroyPipeline(device_->getDevice(), handle_, nullptr);
        }

        [[nodiscard]] VkPipeline Handle() const
        {
            return handle_;
        }

    private:
        Device * device_;
        VkPipeline handle_;
    };
}
