#pragma once

namespace ecs {
    class World;
}

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

        virtual void enable() = 0;
        virtual void disable() = 0;

        virtual void deregisterModule() = 0;
    };
}
