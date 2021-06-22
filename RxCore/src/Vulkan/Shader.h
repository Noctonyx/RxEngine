#pragma once

#include "Vulk.hpp"
#include "Util.h"
#include "Device.h"

namespace RxCore
{
    class Shader
    {
    public:
        explicit Shader(const Device * device, const VkShaderModule & handle)
            : device_(device)
              , handle_(handle)
        {}

        RX_NO_COPY_NO_MOVE(Shader);

        ~Shader()
        {
            vkDestroyShaderModule(device_->getDevice(), handle_, nullptr);
        }

        [[nodiscard]] VkShaderModule Handle() const
        {
            return handle_;
        }

    private:
        const Device * device_;
        VkShaderModule handle_;
    };
}
