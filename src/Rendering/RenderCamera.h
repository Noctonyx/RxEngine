//
// Created by shane on 24/02/2021.
//

#ifndef RX_RENDERCAMERA_H
#define RX_RENDERCAMERA_H

#include <memory>
#include "DirectXMath.h"

namespace RxCore
{
    class Buffer;
    class DescriptorSet;
}

namespace RxEngine
{
    class Camera;
    class Frustum;

    class RenderCamera
    {
    public:
        RenderCamera(std::shared_ptr<Camera> camera);
        void updateDescriptor(
            const std::shared_ptr<RxCore::DescriptorSet> & set,
            uint32_t binding) const;

        uint32_t getDescriptorOffset() const
        {
            return static_cast<uint32_t>((ix_ * bufferAlignment_));
        }

        const std::shared_ptr<Camera> & getCamera() const;

        void readyCameraFrame();
    private:
        std::shared_ptr<Camera> camera_;
        std::shared_ptr<RxCore::Buffer> cameraBuffer2_;
        size_t bufferAlignment_;
        uint32_t ix_;
    };
}
#endif //RX_RENDERCAMERA_H
