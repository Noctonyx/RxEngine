#include "SceneCamera.h"

#include "Window.hpp"
#include "Geometry/Camera.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/Device.h"

#define CAMERA_BUFFER_WINDOW_COUNT 5

namespace RxEngine
{
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
    }

    void SceneCameraModule::shutdown()
    {
        world_->removeSingleton<SceneCamera>();
    }
}
