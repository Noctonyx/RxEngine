#pragma once
#include "DirectXMath.h"
#include "Modules/Module.h"

namespace RxEngine
{
    struct RTSCamera
    {
        float dolly;
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 iView;

        DirectX::XMFLOAT3 viewPos;

        DirectX::XMFLOAT4X4 viewProj;
        DirectX::XMFLOAT4X4 iViewProj;

        [[nodiscard]] DirectX::XMVECTOR getForward() const
        {
            auto v = DirectX::XMVectorSet(0.f, 0.f, -1.f, 0.f);
            v = XMVector3TransformNormal(
                v,
                XMLoadFloat4x4(&iView));
            v = DirectX::XMVector3Normalize(v);
            return v;
        }

        [[nodiscard]] DirectX::XMVECTOR getUp() const
        {
            auto v = DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f);
            v = XMVector3TransformNormal(
                v,
                XMLoadFloat4x4(&iView));
            v = DirectX::XMVector3Normalize(v);
            return v;
        }

        [[nodiscard]] DirectX::XMVECTOR getRight() const
        {
            return DirectX::XMVector3Normalize(
                XMVector3TransformNormal(
                    DirectX::XMVectorSet(1.f, 0.f, 0.f, 0.f),
                    XMLoadFloat4x4(&iView)));
        }
    };

    struct CameraProjection
    {
        DirectX::XMFLOAT4X4 proj;
        float fov;
        float nearZ;
        float farZ;
    };

    class RTSCameraModule : public Module
    {
    public:
        RTSCameraModule(ecs::World * world, EngineMain * engine)
            : Module(world, engine) {}

        void startup() override;
        void shutdown() override;

    private:
        ecs::queryid_t cameraQuery;

        static void updateGui(ecs::EntityHandle e);
    };
}
