#pragma once
#include "DirectXCollision.h"
#include "DirectXMath.h"
#include "imgui.h"
#include "Modules/Module.h"
#include "RxECS.h"

namespace RxEngine
{
    struct RTSCamera
    {
        float dolly;

        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 iView;

        DirectX::XMFLOAT3 viewPos;

        DirectX::XMFLOAT4X4 proj;
        DirectX::XMFLOAT4X4 viewProj;
        DirectX::XMFLOAT4X4 iViewProj;

        [[nodiscard]] DirectX::XMVECTOR getForward() const
        {
            auto v = DirectX::XMVectorSet(0.f, 0.f, -1.f, 0.f);
            v = XMVector3TransformNormal(
                v,
                XMLoadFloat4x4(&iView));
            //v = DirectX::XMVector3Normalize(v);
            return v;
        }

        [[nodiscard]] DirectX::XMVECTOR getUp() const
        {
            auto v = DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f);
            v = XMVector3TransformNormal(
                v,
                XMLoadFloat4x4(&iView));
            //v = DirectX::XMVector3Normalize(v);
            return v;
        }

        [[nodiscard]] DirectX::XMVECTOR getRight() const
        {
            return DirectX::XMVector3Normalize(
                XMVector3TransformNormal(
                    DirectX::XMVectorSet(1.f, 0.f, 0.f, 0.f),
                    XMLoadFloat4x4(&iView)));
        }

        static void rtsCameraUI(ecs::World*, void* ptr)
        {
            auto rts_camera = static_cast<RTSCamera*>(ptr);
            if (rts_camera) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Dolly");
                ImGui::TableNextColumn();
                ImGui::DragFloat("", &rts_camera->dolly);
            }
        }
    };

    struct CameraProjection
    {
        DirectX::XMFLOAT4X4 proj;
        float fov;
        float nearZ;
        float farZ;
    };

    struct CameraFrustum
    {
        DirectX::BoundingFrustum frustum;
    };

    class RTSCameraModule : public Module
    {
    public:
        RTSCameraModule(ecs::World * world, EngineMain * engine, const ecs::entity_t moduleId)
            : Module(world, engine, moduleId) {}

        void startup() override;
        void shutdown() override;

    private:
        ecs::queryid_t cameraQuery;

        static void updateGui(ecs::EntityHandle e);
    };
}
