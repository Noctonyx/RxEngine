//
// Created by shane on 26/01/2021.
//

#ifndef RX_ILIGHTINGMANAGER_H
#define RX_ILIGHTINGMANAGER_H

#include <memory>
#include "Geometry/Camera.hpp"

namespace RxCore
{
    class ImageView;
}

namespace RxEngine
{
    struct ShadowCascade
    {
        DirectX::XMFLOAT4X4 viewProjMatrix;
        //DirectX::XMFLOAT4X4 projMatrix;
        DirectX::XMFLOAT4X4 viewMatrix;
        DirectX::BoundingOrientedBox boBox;
        float splitDepth;
    };

    struct alignas(16) ShadowCascadeShader
    {
        DirectX::XMFLOAT4X4 viewProjMatrix;
//        DirectX::XMFLOAT4X4 projMatrix;
//       DirectX::XMFLOAT4X4 viewMatrix;
        float splitDepth;
        DirectX::XMFLOAT3 pad_;
    };

    struct ILightingManager
    {
        virtual void prepareCamera(const std::shared_ptr<Camera> & camera) = 0;
        virtual void getCascadeData(std::vector<ShadowCascade> & cascades) = 0;
        virtual void setShadowMap(std::shared_ptr<RxCore::ImageView> shadowMap) = 0;
    };
}
#endif //RX_ILIGHTINGMANAGER_H
