#pragma once

#include "RxECS.h"

namespace RxEngine
{
    class EngineMain;

    class Module
    {
    protected:
        ecs::World* world_;
        EngineMain* engine_;

    public:
        Module(ecs::World* world, EngineMain* engine)
            : world_(world),
              engine_(engine)
        {
        }

        virtual ~Module() = default;

        virtual void registerModule() = 0;

        virtual void startup();

        virtual void enable() {};
        virtual void disable() {};

        virtual void shutdown();

        virtual void deregisterModule() = 0;
    };
}
