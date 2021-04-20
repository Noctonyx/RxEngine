//
// Created by shane on 19/02/2021.
//

#include "RmlSystemInterface.h"
#include "Log.h"
#include "EngineMain.hpp"

namespace RxEngine
{
    double RmlSystemInterface::GetElapsedTime()
    {
        return world_->getSingleton<EngineTime>()->totalElapsed;
        //return engine->getTotalElapsed();
    }

    bool RmlSystemInterface::LogMessage(Rml::Log::Type type, const Rml::String & message)
    {
        switch (type) {
        case Rml::Log::LT_ALWAYS:
            spdlog::critical(message);
            break;
        case Rml::Log::LT_DEBUG:
        case Rml::Log::LT_MAX:
            spdlog::debug(message);
            break;
        case Rml::Log::LT_ERROR:
        case Rml::Log::LT_ASSERT:
            spdlog::error(message);
            break;
        case Rml::Log::LT_INFO:
            spdlog::info(message);
            break;
        case Rml::Log::LT_WARNING:
            spdlog::warn(message);
            break;
        }
        return true;
    }

    RmlSystemInterface::RmlSystemInterface(ecs::World * world)
        : world_(world) {}

    int RmlSystemInterface::TranslateString(Rml::String & translated, const Rml::String & input)
    {
        return SystemInterface::TranslateString(translated, input);
    }
}
