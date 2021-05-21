#include "CameraControl.h"

#include "EngineMain.hpp"
#include "Window.hpp"
#include "Modules/RTSCamera/RTSCamera.h"
#include "Modules/SceneCamera/SceneCamera.h"
#include "Modules/Transforms/Transforms.h"

using namespace DirectX;

namespace RxEngine
{
    void CameraControlModule::startup()
    {
        world_->createSystem("Camera:Scroll")
              .inGroup("Pipeline:Update")
              .withStream<MouseScroll>()
              .withWrite<RTSCamera>()
              .execute<MouseScroll>([](ecs::World * world, const MouseScroll * ms)
              {
                  const auto scene_camera = world->getSingleton<SceneCamera>();
                  const auto rts_camera = world->getUpdate<RTSCamera>(scene_camera->camera);

                  const auto arm_delta = ms->y_offset * world->deltaTime() * 25.0f;
                  rts_camera->dolly = std::clamp(rts_camera->dolly + arm_delta, 5.0f, 200.0f);

                  return true;
              });

        world_->createSystem("Camera:RightButton")
              .inGroup("Pipeline:Update")
              .withStream<MouseButton>()
              .execute<MouseButton>([this](ecs::World * world, const MouseButton * mb)
              {
                  auto window = engine_->getWindow();

                  if (mb->button == 1) {
                      window->setMouseVisible(!mb->pressed);
                      movingCamera = mb->pressed;
                  }
                  return true;
              });

        world_->createSystem("Camera:MoveMouse")
            .inGroup("Pipeline:Update")
            .withStream<MousePosition>()
            .execute<MousePosition>([this](ecs::World* world, const MousePosition* mp)
                {
                    if (!movingCamera) {
                        return false;
                    }
                    auto sc = world->getSingleton<SceneCamera>();
                    if (!sc) {
                        return false;
                    }
                    auto delta = world->deltaTime();
                    const auto delta_rot = XMVectorSet(-mp->deltaY* delta,
                        -mp->deltaX * delta, 0, 0);

                    spdlog::info("Mouse delta {0} {1}, {2}", delta, mp->deltaX, mp->deltaY);
                    auto lr = world->getUpdate<Transforms::LocalRotation>(sc->camera);
                    if (!lr) {
                        return false;
                    }
                    XMVECTOR rot = XMLoadFloat3(&lr->rotation);
                    rot = XMVectorAdd(rot, delta_rot);
                    rot = XMVectorClamp(
                        rot,
                        XMVectorSet(-XM_PIDIV2, -XM_2PI, -XM_2PI, 0.f),
                        XMVectorSet(-0.05f, XM_2PI, XM_2PI, 0.f)
                    );

                    XMStoreFloat3(&lr->rotation, XMVectorModAngles(rot));
                    return true;
                });
#if 0
        world_->createSystem("Camera:Rotate")
              .inGroup("Pipeline:Update")
              .withRead<MouseStatus>()
              .execute([&movingCamera](ecs::World * world)
              {
                  if(!movingCamera) {
                      return;
                  }

                  auto delta = world->deltaTime();

                  auto ms = world->getSingleton<MouseStatus>();
                  auto sc = world->getSingleton<SceneCamera>();
                  if (!sc) {
                      return;
                  }

                  if (!ms || !ms->button2) {
                      return;
                  }

                  const auto delta_rot = XMVectorSet(-ms->deltaMouseY * delta,
                                                    -ms->deltaMouseX * delta, 0, 0);

                  auto lr = world->getUpdate<Transforms::LocalRotation>(sc->camera);
                  if (!lr) {
                      return;
                  }
                  XMVECTOR rot = XMLoadFloat3(&lr->rotation);
                  rot = XMVectorAdd(rot, delta_rot);
                  rot = XMVectorClamp(
                      rot,
                      XMVectorSet(-XM_PIDIV2, -XM_2PI, -XM_2PI,0.f),
                      XMVectorSet(-0.05f, XM_2PI, XM_2PI, 0.f)
                  );

                  XMStoreFloat3(&lr->rotation, XMVectorModAngles(rot));
              });
#endif
    }

    void CameraControlModule::shutdown() { }
}
