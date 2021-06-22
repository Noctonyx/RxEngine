#include "Events.h"
#include "SDL.h"

namespace RxCore
{
    void Events::startup()
    {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    }

    void Events::shutdown()
    {
        SDL_Quit();
    }

    void Events::pollEvents(std::function<void(SDL_Event * ev)> f)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {

            f(&event);
        }
    }

    SDL_Keymod Events::getKeyboardMods()
    {
        SDL_Keymod km =  SDL_GetModState();
        return km;
    }
}
