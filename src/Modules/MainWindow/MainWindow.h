#pragma once

#include <cstdint>

#include "Modules/Module.h"

namespace RxEngine
{
    class Window;

    namespace MainWindow
    {
        struct WindowDetails
        {
            Window * window;

            uint32_t width;
            uint32_t height;
        };
    }

    class MainWindowModule : public Module
    {
    public:
        void registerModule() override;
        void startup() override;
        void shutdown() override;
        void deregisterModule() override;
    };
}
