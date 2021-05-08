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
              .each<Transforms::WorldPosition, Transforms::YRotation, WorldTransform>(
                  [](ecs::EntityHandle e,
                     const Transforms::WorldPosition * wp,
                     const Transforms::YRotation * yrot,
                     WorldTransform * wt)
                  {
                      auto tm = XMMatrixTranslation(wp->position.x, wp->position.y, wp->position.z);
                      auto rm = XMMatrixRotationY(yrot ? yrot->yRotation : 0.0f);
                      auto nm = XMMatrixMultiply(rm, tm);
                      if (!wt) {
                          WorldTransform wtt;
                          XMStoreFloat4x4(&wtt.transform, nm);

                          e.addDeferred<WorldTransform>();
                          e.setDeferred<WorldTransform>(std::move(wtt));
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
