#include "WorldObject.h"

#include "Modules/Transforms/Transforms.h"

using namespace DirectX;

namespace RxEngine
{
    void WorldObjectModule::startup()
    {
        world_->createSystem("WorldObject:CreateTransform")
              .withQuery<WorldObject, Transforms::WorldPosition>()
              .inGroup("Pipeline:PostUpdate")
              .each<Transforms::WorldPosition, Transforms::LocalRotation, WorldTransform>(
                  [](ecs::EntityHandle e,
                     const Transforms::WorldPosition * wp,
                     const Transforms::LocalRotation * rot,
                     WorldTransform * wt)
                  {
                      auto tm = XMMatrixTranslation(wp->position.x, wp->position.y, wp->position.z);
                      //auto rm = XMMatrixRotationRollPitchYaw(rot ? rot->rotation.x : 0.0f, rot ? rot->rotation.y : 0.0f, rot ? rot->rotation.z : 0.0f);
                      auto rm = XMMatrixIdentity();
                      auto nm = XMMatrixMultiply(rm, tm);
                      if (!wt) {
                          WorldTransform wtt;
                          XMStoreFloat4x4(&wtt.transform, nm);

                          e.addDeferred<WorldTransform>();
                          e.setDeferred<WorldTransform>(wtt);
                          return;
                      }
                      XMStoreFloat4x4(&wt->transform, nm);
                  });
    }

    void WorldObjectModule::shutdown()
    {
        world_->deleteSystem(world_->lookup("WorldObject:CreateTransform"));
    }
}
