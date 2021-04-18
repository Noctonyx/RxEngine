#pragma once

#include <cstdint>

#include "RxECS.h"
#include "Input/Keyboard.hpp"
#include "Modules/Module.h"

namespace RxEngine
{
    struct KeyboardChar
    {
        char c;
    };

    struct KeyboardKey
    {
        EKey key;
        EInputAction action;
        EInputMod mods;
    };

    class KeyboardModule : public Module
    {
    public:
        void registerModule() override;
        void startup() override;
        void shutdown() override;
        void deregisterModule() override;

        void onKey(EKey key, EInputAction action, EInputMod mods);
        void onChar(char c);
    };
}
