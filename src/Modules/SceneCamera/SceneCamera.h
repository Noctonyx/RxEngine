#pragma once
#include "Modules/Module.h"

namespace RxEngine
{
    class Camera;

    struct CameraProjection
    {
        
    };

    struct SceneCamera
    {
        std::shared_ptr<Camera> camera;

        
    };

    class SceneCameraModule: public Module
    {
    public:
        void startup() override;
        void shutdown() override;
    };
}
