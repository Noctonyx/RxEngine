#pragma once

#include "Vulk.hpp"
#include "Device.h"

namespace RxCore
{
    class FrameBuffer
    {
    public:
        FrameBuffer(Device * device, const VkFramebuffer & handle)
            : device_(device)
              , handle_(handle)
        {}

        FrameBuffer(const FrameBuffer & other) = delete;

        FrameBuffer(FrameBuffer && other) noexcept = delete;

        FrameBuffer & operator=(const FrameBuffer & other) = delete;

        FrameBuffer & operator=(FrameBuffer && other) noexcept = delete;

        ~FrameBuffer()
        {
            vkDestroyFramebuffer(device_->getDevice(), handle_, nullptr);
        }

        [[nodiscard]] VkFramebuffer Handle() const
        { return handle_; }

    private:
        Device * device_;
        VkFramebuffer handle_;
    };
}
