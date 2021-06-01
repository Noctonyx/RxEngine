#pragma once

#include <deque>

#include "Modules/Module.h"
#include "RxECS.h"
#include "Vulkan/Device.h"

namespace RxEngine
{
    class EngineMain;

    class StatsModule : public Module
    {
        float delta_ = 0.f;
        float fps_ = 0.f;
        std::deque<float> fpsHistory_{};
        std::deque<float> gpuHistory_{};

        std::vector<RxCore::MemHeapStatus> heaps_{};

    public:
        StatsModule(ecs::World * world, EngineMain * engine)
            : Module(world, engine) {}

        void startup() override;
        void shutdown() override;

    protected:
        void presentStatsUi();
    };
}
