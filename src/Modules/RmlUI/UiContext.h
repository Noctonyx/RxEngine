//
// Created by shane on 21/02/2021.
//

#pragma once

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Input.h>
#include "Window.hpp"
#include "Modules/Module.h"

namespace RxEngine
{
    enum class EInputMod;

    struct UiContextProcessed{};

    struct MainUiContext
    {
        Rml::Context * context;
        std::string contextName{};
        std::vector<std::pair<std::string, Rml::ElementDocument*>> documents{};

        Rml::ElementDocument * loadDocument(const std::string & document);
        void closeDocument(const std::string & document);
    };
   
    class UiContext : public Module
    {
    public:
        UiContext(ecs::World* world, EngineMain* engine);
        virtual ~UiContext();

        void startup() override;
        void shutdown() override;

        Rml::ElementDocument * addDocument(const std::string & document);

        static int convertModState(RxEngine::EInputMod mods);
        static Rml::Input::KeyIdentifier convertKey(EKey key);

    private:
        //Rml::Context * context_;
        //std::vector<std::pair<std::string, Rml::ElementDocument *>> documents_{};
    };
}
