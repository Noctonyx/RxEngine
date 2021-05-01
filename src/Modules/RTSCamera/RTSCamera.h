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
    };
}
