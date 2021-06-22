#pragma once
#include <functional>
#include "SDL_events.h"

namespace RxCore
{
    class Events
    {
    public:
        static void startup();
        static void shutdown();
        static void pollEvents(std::function<void(SDL_Event* ev)> f);
        static SDL_Keymod getKeyboardMods();
    };
}
