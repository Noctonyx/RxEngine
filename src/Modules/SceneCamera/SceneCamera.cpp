////////////////////////////////////////////////////////////////////////////////
// MIT License
//
// Copyright (c) 2021.  Shane Hyde
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

#include <Modules/Renderer/Renderer.hpp>
#include "SceneCamera.h"

#include "EngineMain.hpp"
#include "Geometry/Camera.hpp"
#include "Modules/RTSCamera/RTSCamera.h"

constexpr int camera_buffer_count = 5;

namespace RxEngine
{
    void sceneCameraGUI(ecs::World * w, void * ptr)
    {
        auto sc = static_cast<SceneCamera *>(ptr);
        if (sc) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Index");
            ImGui::TableNextColumn();
            ImGui::Text("%d", sc->ix);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Camera");
            ImGui::TableNextColumn();
            if (w->isAlive(sc->camera)) {
                ImGui::Text("%s", w->description(sc->camera).c_str());
            } else {
                ImGui::Text("%lld", sc->camera);
            }
        }
    }

    void SceneCameraModule::startup()
    {
        world_->addSingleton<SceneCamera>();
        auto sc = world_->getSingletonUpdate<SceneCamera>();

        auto device = engine_->getDevice();

        //sc->bufferAlignment = engine_->getUniformBufferAlignment(sizeof(SceneCameraShaderData));
        sc->camBuffer = engine_->createUniformDynamicBuffer(sizeof(SceneCameraShaderData), camera_buffer_count);
        sc->camBuffer->map();
        sc->ix = 0;

        world_->createSystem("SceneCamera:NextFrame")
              .inGroup("Pipeline:PreRender")
              .withWrite<SceneCamera>()
              .withRead<RTSCamera>()
              .execute([](ecs::World * world)
                  {
                      const auto sc = world->getSingletonUpdate<SceneCamera>();
                      const auto c = world->get<RTSCamera>(sc->camera);

                      sc->shaderData.view = c->view;
                      sc->shaderData.projection = c->proj;
                      sc->shaderData.viewPos = c->viewPos;

                      sc->ix = (sc->ix + 1) % camera_buffer_count;
                      sc->camBuffer->update(&sc->shaderData,
                                            sc->ix * sc->camBuffer->getAlignment(),
                                            sizeof(SceneCameraShaderData));
                  }
              );

        world_->createSystem("SceneCamera:setDescriptor")
              .inGroup("Pipeline:PreFrame")
              .withQuery<DescriptorSet>()
              .without<SceneCameraDescriptor>()
              .withSingleton<SceneCamera>()
              .each<DescriptorSet, SceneCamera>(
                  [](ecs::EntityHandle e, DescriptorSet * ds, const SceneCamera * sc)
                  {
                      ds->ds->updateDescriptor(0, vk::DescriptorType::eUniformBufferDynamic,
                                               sc->camBuffer, sizeof(SceneCameraShaderData),
                                               static_cast<uint32_t>(sc->ix * sc->bufferAlignment));

                      e.addDeferred<SceneCameraDescriptor>();
                  });

        world_->createSystem("SceneCamera:updateDescriptor")
              .inGroup("Pipeline:PreRender")
              .withQuery<DescriptorSet, SceneCameraDescriptor>()
              .withSingleton<SceneCamera>()
              .each<DescriptorSet, SceneCamera>(
                  [](ecs::EntityHandle e, const DescriptorSet * ds, const SceneCamera * sc)
                  {
                      ds->ds->setDescriptorOffset(0, sc->getDescriptorOffset());
                  });
#if 0
        world_->createSystem("SceneCamera:UpdateDescriptor")
            .inGroup("Pipeline:PreRender")
            .withWrite<Descriptors>()
            .withRead<Descriptors>()
            .withRead<SceneCamera>()
            .execute([](ecs::World* world)
                {
                    auto ds = world->getSingletonUpdate<Descriptors>();
                    auto sc = world->getSingleton<SceneCamera>();
                
                    ds->ds0->setDescriptorOffset(0, sc->getDescriptorOffset());
                });
#endif
        world_->set<ComponentGui>(world_->getComponentId<SceneCamera>(),
                                  {sceneCameraGUI});
    }

    void SceneCameraModule::shutdown()
    {
        world_->removeSingleton<SceneCamera>();
    }
}
