////////////////////////////////////////////////////////////////////////////////
// MIT License
//
// Copyright (c) 2021.  Shane Hyde (shane@noctonyx.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

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
              //.withJob()
              .withUpdates()
              .each<Transforms::WorldPosition, Transforms::LocalRotation, WorldTransform>(
                  [](ecs::EntityHandle e,
                     const Transforms::WorldPosition * wp,
                     const Transforms::LocalRotation * rot,
                     WorldTransform * wt)
                  {
                      auto tm = XMMatrixTranslation(wp->position.x, wp->position.y, wp->position.z);
                      auto rm = XMMatrixRotationRollPitchYaw(
                          rot ? rot->rotation.x : 0.0f, rot ? rot->rotation.y : 0.0f,
                          rot ? rot->rotation.z : 0.0f);
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
