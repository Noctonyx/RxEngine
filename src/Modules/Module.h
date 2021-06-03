#pragma once

#include "RxECS.h"
#include "sol/table.hpp"

namespace RxCore
{
    class Device;
}

namespace sol
{
    class state;
}

namespace RxEngine
{
    class EngineMain;

    class Module : public ecs::ModuleBase
    {
    protected:
        EngineMain * engine_;

    public:
        Module(ecs::World * world, EngineMain * engine, const ecs::entity_t moduleId)
            : ecs::ModuleBase(world, moduleId)
            , engine_(engine)
        {           
        }

        virtual void startup() {}
        virtual void loadData(sol::table table) {}
        virtual void shutdown() {}


        //virtual void processStartupData(sol::state* lua, RxCore::Device* device) {};
    };
}
