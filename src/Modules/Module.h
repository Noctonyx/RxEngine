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
            , moduleId(moduleId)
        {
            world->setModuleObject(moduleId, this);
        }

        virtual ~Module() = default;

        virtual void startup() {}
        virtual void loadData(sol::table table) {}
        virtual void shutdown() {}

        virtual void onDisabled() {}
        virtual void onEnabled() {}

        void enable()
        {
            world_->setModuleEnabled(moduleId, true);
            onEnabled();
        }

        void disable()
        {
            world_->setModuleEnabled(moduleId, false);
            onDisabled();
        };

        ecs::entity_t getModuleId() const { return moduleId; }

        template<class T>
        T* getObject();

        //virtual void processStartupData(sol::state* lua, RxCore::Device* device) {};
    };

    template <class T>
    T * Module::getObject()
    {
        return world_->getModuleObject<T>(moduleId);
    }
}
