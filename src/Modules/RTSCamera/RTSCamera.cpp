#include "RTSCamera.h"

#include "Modules/Transforms/Transforms.h"

using namespace RxEngine::Transforms;
using namespace DirectX;

namespace RxEngine
{
    void RTSCameraModule::startup()
    {
        world_->createSystem("RTSCamera:CalculateMatrix")
              .inGroup("Pipeline:PreRender")
              .withQuery<RTSCamera>()
              .with<WorldPosition, YRotation, XRotation, CameraProjection>()
              .each<RTSCamera, WorldPosition, YRotation, XRotation, CameraProjection>(
                  [](ecs::EntityHandle e,
                     RTSCamera * c,
                     const WorldPosition * wp,
                     const YRotation * yrot,
                     const XRotation * xrot,
                     const CameraProjection * proj
              )
                  {
                      XMFLOAT3 rot(xrot->xRotation, yrot->yRotation, 0.f);
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
                      XMStoreFloat4x4(&c->iViewProj, XMMatrixInverse(nullptr, viewProj));
                  });

        cameraQuery = world_->createQuery().with<CameraProjection>().id;


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

        CameraProjection def;
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
              .add<XRotation>()
              .add<YRotation>()
              .add<RTSCamera>()
              .set<CameraProjection>(def)
              .add<ecs::Prefab>();
    }

    void RTSCameraModule::shutdown()
    {
        world_->deleteSystem(world_->lookup("RTSCamera:CalculateMatrix").id);
    }
}
