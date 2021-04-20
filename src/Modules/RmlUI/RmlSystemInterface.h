//
// Created by shane on 19/02/2021.
//

#pragma once

#include <RmlUi/Core/SystemInterface.h>

namespace ecs
{
    class World;
}

namespace RxEngine
{
    class EngineMain;

    class RmlSystemInterface : public Rml::SystemInterface
    {
    public:
        RmlSystemInterface(ecs::World * world);

        bool LogMessage(Rml::Log::Type type, const Rml::String & message) override;
        double GetElapsedTime() override;
        int TranslateString(Rml::String & translated, const Rml::String & input) override;

    private:
        ecs::World * world_;
    };
}

