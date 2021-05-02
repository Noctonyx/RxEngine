#include "Environment.h"

namespace RxEngine
{
    void EnvironmentModule::startup()
    {
        world_->setSingleton<SunDirection>({ {0.f, 1.f, 0.f} });
    }
    void EnvironmentModule::shutdown()
    {
        world_->removeSingleton<SunDirection>();
    }
}
