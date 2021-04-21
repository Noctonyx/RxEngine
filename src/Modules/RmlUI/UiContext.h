//
// Created by shane on 21/02/2021.
//

#pragma once

#include <Rendering/Renderer.hpp>
#include <Subsystem.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Input.h>

#include "Input/Keyboard.hpp"
#include "Modules/Module.h"

namespace RxEngine
{
    enum class EInputMod;
   
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
        Rml::Context * context_;
        std::vector<Rml::ElementDocument *> documents_{};
    };
}
