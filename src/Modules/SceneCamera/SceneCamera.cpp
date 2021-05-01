#include "SceneCamera.h"

#include "Window.hpp"
#include "Geometry/Camera.hpp"

namespace RxEngine
{
    void SceneCameraModule::startup()
    {
        auto camera = std::make_shared<Camera>();
        world_->setSingleton<SceneCamera>({.camera= camera});
        auto wd = world_->getSingleton<WindowDetails>();
        assert(wd);

        camera->setPerspective(
            60.0f, static_cast<float>(wd->width) / static_cast<float>(wd->height), 0.1f, 256.f);


    }

    void SceneCameraModule::shutdown() {}
}
