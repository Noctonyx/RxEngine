#pragma once

#include "Modules/Module.h"

namespace RxEngine
{
    class CameraControlModule : public Module
    {
    public:
        CameraControlModule(ecs::World * world, EngineMain * engine)
            : Module(world, engine) {}

        void startup() override;
        void shutdown() override;

    private :
        bool movingCamera = false;
        bool forwardDown_ = false;
        bool backwardDown_ = false;
        bool rightDown_ = false;
        bool leftDown_ = false;
    };
}
