#pragma once

#include <deque>

#include "Modules/Module.h"
#include "RxECS.h"

namespace RxEngine
{
    class EngineMain;

    class StatsModule: public Module
    {
        ecs::entity_t systemSet_;

        float delta_;
        float fps_;
        std::deque<float> fpsHistory_;
        std::deque<float> gpuHistory_;

    public:
        void registerModule() override;
        void enable() override;
        void disable() override;
        void deregisterModule() override;

    protected:
        void presentStatsUi();
    };
}
