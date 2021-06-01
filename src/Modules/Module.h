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

    class Module
    {
    protected:
        ecs::World * world_;
        EngineMain * engine_;
        const ecs::entity_t moduleId;

    public:
        Module(ecs::World * world, EngineMain * engine, const ecs::entity_t moduleId)
            : world_(world)
            , engine_(engine)
            , moduleId(moduleId) { }

        virtual ~Module() = default;

        virtual void registerModule() {}

        virtual void startup() {}
        virtual void loadData(sol::table table) {}

        //virtual void enable() {};
        //virtual void disable() {};

        virtual void shutdown() {}

        virtual void deregisterModule() {}

        void enable() const
        {
            world_->setModuleEnabled(moduleId, true);
        }

        void disable() const
        {
            world_->setModuleEnabled(moduleId, false);
        };

        ecs::entity_t getModuleId() const { return moduleId; }

        //virtual void processStartupData(sol::state* lua, RxCore::Device* device) {};
    };
}
