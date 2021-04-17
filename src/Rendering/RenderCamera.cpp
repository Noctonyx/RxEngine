//
// Created by shane on 24/02/2021.
//

#include "RenderCamera.h"

#include <utility>
#include "Vulkan/Device.h"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Geometry/Camera.hpp"

#define CAMERA_BUFFER_WINDOW_COUNT 5

namespace RxEngine
{
    RenderCamera::RenderCamera(std::shared_ptr<Camera> camera)
        : camera_(std::move(camera))
        , ix_(0)
    {
        bufferAlignment_ = RxCore::iVulkan()->getUniformBufferAlignment(sizeof(CameraShaderData));

        cameraBuffer2_ = RxCore::iVulkan()->createBuffer(
            vk::BufferUsageFlagBits::eUniformBuffer,
            VMA_MEMORY_USAGE_CPU_TO_GPU, CAMERA_BUFFER_WINDOW_COUNT * bufferAlignment_);
        cameraBuffer2_->getMemory()->map();
    }

    void RenderCamera::updateDescriptor(
        const std::shared_ptr<RxCore::DescriptorSet> & set,
        uint32_t binding) const
    {
        set->updateDescriptor(
            binding, vk::DescriptorType::eUniformBufferDynamic, cameraBuffer2_,
            sizeof(CameraShaderData), static_cast<uint32_t>( ix_ * bufferAlignment_));
    }

    const std::shared_ptr<Camera> & RenderCamera::getCamera() const
    {
        return camera_;
    }

    void RenderCamera::readyCameraFrame()
    {
        ix_ = (ix_ + 1) % CAMERA_BUFFER_WINDOW_COUNT;

        cameraBuffer2_->getMemory()
                      ->update(&(camera_->cameraShaderData), ix_ * bufferAlignment_, sizeof(CameraShaderData));
    }
}