#include "SceneCamera.h"

#include "EngineMain.hpp"
#include "imgui.h"
#include "Window.hpp"
#include "Geometry/Camera.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/Device.h"

#define CAMERA_BUFFER_WINDOW_COUNT 5

namespace RxEngine
{
    void sceneCameraGUI(ecs::World * w, void * ptr)
    {
        auto sc = static_cast<SceneCamera*>(ptr);
        if(sc) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Index");
            ImGui::TableNextColumn();
            ImGui::Text("%d", sc->ix);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Camera");
            ImGui::TableNextColumn();
            if(w->isAlive(sc->camera)) {
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

        sc->bufferAlignment = RxCore::iVulkan()->
            getUniformBufferAlignment(sizeof(CameraShaderData));
        sc->camBuffer = RxCore::iVulkan()->createBuffer(
            vk::BufferUsageFlagBits::eUniformBuffer,
            VMA_MEMORY_USAGE_CPU_TO_GPU, CAMERA_BUFFER_WINDOW_COUNT * sc->bufferAlignment);
        sc->camBuffer->getMemory()->map();
        sc->ix = 0;

        world_->createSystem("SceneCamera:NextFrame")
              .inGroup("Pipeline:PreRender")
              .execute([](ecs::World * world)
                  {
                      const auto sc = world->getSingletonUpdate<SceneCamera>();
                      sc->ix = (sc->ix + 1) % CAMERA_BUFFER_WINDOW_COUNT;
                  }
              );

        world_->set<ComponentGui>(world_->getComponentId<SceneCamera>(), { .editor = sceneCameraGUI });
    }

    void SceneCameraModule::shutdown()
    {
        world_->removeSingleton<SceneCamera>();
    }
}
