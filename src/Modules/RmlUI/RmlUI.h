#pragma once
#include "Modules/Module.h"
#include "RmlFileInterface.h"
#include "RmlRenderInterface.h"
#include "RmlSystemInterface.h"

namespace RxEngine 
{
    class RmlUiModule: public Module
    {
    public:
        RmlUiModule(ecs::World * world, EngineMain * engine)
            : Module(world, engine) {}

        void registerModule() override;
        void startup() override;
        void shutdown() override;
        void deregisterModule() override;

    protected:

        std::unique_ptr<RmlSystemInterface> rmlSystem;
        std::unique_ptr<RmlFileInterface> rmlFile;
        std::unique_ptr<RmlRenderInterface> rmlRender;
    };
}
