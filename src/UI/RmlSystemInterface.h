//
// Created by shane on 19/02/2021.
//

#ifndef RX_RMLSYSTEMINTERFACE_H
#define RX_RMLSYSTEMINTERFACE_H

#include <RmlUi/Core/SystemInterface.h>

namespace RxEngine
{
    class EngineMain;

    class RmlSystemInterface : public Rml::SystemInterface
    {
    public:
        RmlSystemInterface(EngineMain * engine);

        bool LogMessage(Rml::Log::Type type, const Rml::String & message) override;
        double GetElapsedTime() override;
        int TranslateString(Rml::String & translated, const Rml::String & input) override;

    private:
        EngineMain * engine;
    };
}
#endif //RX_RMLSYSTEMINTERFACE_H
