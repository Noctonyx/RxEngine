#pragma once

#include <deque>

#include "Modules/Module.h"
#include "RxECS.h"

namespace RxEngine
{
    class EngineMain;

    class StatsModule: public Module
    {
        ecs::World* world_;
        EngineMain* engine_;

        ecs::entity_t systemSet_;

        float delta_;
        float fps_;
        std::deque<float> fpsHistory_;
        std::deque<float> gpuHistory_;

    public:
        StatsModule() = default;
        void registerModule(EngineMain* engine, ecs::World* world) override;
        void enable() override;
        void disable() override;
        void unregisterModule() override;

    protected:
        void presentStatsUi();
    };
}
