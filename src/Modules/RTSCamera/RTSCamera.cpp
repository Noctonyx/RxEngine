#include "RTSCamera.h"

#include "RxECS.h"
#include "imgui.h"
#include "Modules/Transforms/Transforms.h"
#include "Modules/ImGui/ImGuiRender.hpp"

using namespace RxEngine::Transforms;
using namespace DirectX;

namespace RxEngine
{
    void RTSCameraModule::startup()
    {
        world_->createSystem("RTSCamera:CalculateMatrix")
              .inGroup("Pipeline:PreRender")
              .withQuery<RTSCamera, WorldPosition, LocalRotation, CameraProjection,
                         CameraFrustum>()
              .each<RTSCamera, WorldPosition, LocalRotation, CameraProjection,
                    CameraFrustum>(
                  [](ecs::EntityHandle e,
                     RTSCamera * c,
                     const WorldPosition * wp,
                     const LocalRotation * rotv,
                     const CameraProjection * proj,
                     CameraFrustum * fru
              )
                  {
                      OPTICK_EVENT("Update Camera")
                      XMFLOAT3 rot(rotv->rotation);
                      XMFLOAT3 dolly(0.f, 0.f, c->dolly);

                      XMMATRIX rotM = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&rot));
                      XMMATRIX transM = XMMatrixTranslationFromVector(XMLoadFloat3(&wp->position));
                      XMMATRIX dollyM = XMMatrixTranslationFromVector(XMLoadFloat3(&dolly));

                      auto iView = dollyM * rotM * transM;
                      auto view = XMMatrixInverse(nullptr, iView);

                      XMStoreFloat4x4(&c->view, view);
                      XMStoreFloat4x4(&c->iView, iView);

                      XMStoreFloat3(
                          &c->viewPos,
                          XMVector3TransformCoord(
                              XMVectorZero(),
                              iView
                          )
                      );

                      auto pr = XMLoadFloat4x4(&proj->proj);
                      auto viewProj = view * pr;

                      XMStoreFloat4x4(&c->viewProj, viewProj);
                      XMStoreFloat4x4(&c->proj, pr);
                      XMStoreFloat4x4(&c->iViewProj, XMMatrixInverse(nullptr, viewProj));

                      const BoundingFrustum frustum(pr, true);

                      frustum.Transform(fru->frustum, XMMatrixInverse(nullptr, view));
                  });

        cameraQuery = world_->createQuery<CameraProjection>().id;

        world_->createSystem("RTSCamera:AspectRatio")
              .inGroup("Pipeline:Update")
              .withStream<WindowResize>()
              .execute<WindowResize>([this](ecs::World * world, const WindowResize * res)
              {
                  world->getResults(cameraQuery)
                       .each<CameraProjection>([&res](ecs::EntityHandle e, CameraProjection * pr)
                       {
                           XMStoreFloat4x4(
                               &pr->proj,
                               XMMatrixPerspectiveFovRH(
                                   XMConvertToRadians(pr->fov),
                                   static_cast<float>(res->width) / static_cast<float>(res->height),
                                   pr->nearZ,
                                   pr->farZ)
                           );
                       });
                  return false;
              });

        CameraProjection def{};
        def.fov = 60.f;
        def.farZ = 256.0f;
        def.nearZ = 0.1f;

        auto wd = world_->getSingleton<WindowDetails>();

        XMStoreFloat4x4(
            &def.proj,
            XMMatrixPerspectiveFovRH(
                XMConvertToRadians(def.fov),
                static_cast<float>(wd->width) / static_cast<float>(wd->height),
                def.nearZ,
                def.farZ)
        );

        world_->newEntity("PrefabRTSCamera")
              .add<WorldPosition>()
              .add<LocalRotation>()
              .add<RTSCamera>()
              .set<CameraProjection>(def)
              .add<CameraFrustum>()
              .add<ecs::Prefab>();

        world_->createSystem("RTSCamera:Stats")
              .inGroup("Pipeline:UpdateUi")
              .withQuery<RTSCamera>()
              .each<>([](ecs::EntityHandle e)
              {
                  updateGui(e);
              });

        world_->set<ComponentGui>(world_->getComponentId<RTSCamera>(),
                                  {.editor = RTSCamera::rtsCameraUI});
    }

    void RTSCameraModule::shutdown()
    {
        world_->deleteSystem(world_->lookup("RTSCamera:CalculateMatrix").id);
        world_->deleteSystem(world_->lookup("RTSCamera:Stats").id);
        world_->deleteSystem(world_->lookup("RTSCamera:AspectRatio").id);

        world_->remove<ComponentGui>(world_->getComponentId<RTSCamera>());
    }

    void RTSCameraModule::updateGui(ecs::EntityHandle e)
    {
        auto camera = e.get<RTSCamera>();
        auto pos = e.get<WorldPosition>();
        auto xrot = e.get<LocalRotation>()->rotation.x;
        auto yrot = e.get<LocalRotation>()->rotation.y;

        const float DISTANCE = 5.0f;

        auto & io = ImGui::GetIO();
        if(io.UserData && !static_cast<IMGuiRender *>(io.UserData)->isEnabled()) {
            return;
        }
        ImVec2 window_pos = ImVec2(
            io.DisplaySize.x - 2 * DISTANCE,
            io.DisplaySize.y - 2 * DISTANCE);

        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(1.0f, 1.0f));
        ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
        if (ImGui::Begin(
            "Camera Overlay",
            nullptr,
            (ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                ImGuiWindowFlags_NoNav))) {

            DirectX::XMFLOAT3 fwd;
            DirectX::XMFLOAT3 up;
            DirectX::XMFLOAT3 right;
            DirectX::XMStoreFloat3(&fwd, camera->getForward());
            DirectX::XMStoreFloat3(&up, camera->getUp());
            DirectX::XMStoreFloat3(&right, camera->getRight());
            ImGui::Text(
                "Base Position: %.3f, %.3f, %.3f", pos->position.x,
                pos->position.y,
                pos->position.z);
            ImGui::Text(
                "Cam Position: %.3f, %.3f, %.3f", camera->viewPos.x,
                camera->viewPos.y,
                camera->viewPos.z);
            ImGui::Text(
                "X/Y/Z Rotation: %.3f, %.3f, %.3f",
                xrot,
                yrot,
                0.f);
            ImGui::Text("Arm Length: %.3f", camera->dolly);
            ImGui::Text("Forward: %.3f, %.3f, %.3f", fwd.x, fwd.y, fwd.z);
            ImGui::Text("Up: %.3f, %.3f, %.3f", up.x, up.y, up.z);
            ImGui::Text("Right: %.3f, %.3f, %.3f", right.x, right.y, right.z);
        }
        ImGui::End();
    }
}
