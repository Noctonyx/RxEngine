#include "Prototypes.h"

#include "sol/state.hpp"
#include "sol/table.hpp"

namespace RxEngine
{
    void PrototypesModule::startup() { }

    void PrototypesModule::shutdown() { }

    void loadPrototype(ecs::World * world,
                       RxCore::Device * device,
                       std::string prototypeName,
                       sol::table details) { }

    void loadPrototypes(ecs::World * world, RxCore::Device * device, sol::table & prototypes)
    {
        for (auto & [key, value]: prototypes) {
            const std::string name = key.as<std::string>();
            sol::table details = value;
            loadPrototype(world, device, name, details);
        }
    }

    void PrototypesModule::processStartupData(sol::state * lua, RxCore::Device * device)
    {
        sol::table data = lua->get<sol::table>("data");

        sol::table prototypes = data.get<sol::table>("prototypes");

        loadPrototypes(world_, device, prototypes);
    }
}
