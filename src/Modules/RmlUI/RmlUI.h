#pragma once
#include "Modules/Module.h"
#include "RmlFileInterface.h"
#include "RmlRenderInterface.h"
#include "RmlSystemInterface.h"

namespace Rml {
    class ElementDocument;
}

namespace RxEngine 
{
    struct UiContext
    {
        Rml::Context* context;
        bool debugger = false;
        //bool interactive = false;
        std::vector<std::pair<std::string, Rml::ElementDocument*>> documents{};

        Rml::ElementDocument* loadDocument(const std::string& document);
        void closeDocument(const std::string& document);
    };

    struct DestroyUi {};
    struct UiContextCreated {};
    struct UiContextInteractive {};
    struct UiContextProcessed {};

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
