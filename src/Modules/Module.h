#pragma once

namespace ecs {
    class World;
}

namespace RxEngine
{
    class EngineMain;

    class Module
    {
    public:
        virtual void registerModule(EngineMain* engine, ecs::World * world) = 0;

        virtual void enable() = 0;
        virtual void disable() = 0;

        virtual void unregisterModule() = 0;
    };
}
