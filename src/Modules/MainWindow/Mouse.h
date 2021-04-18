#pragma once

#include <cstdint>

#include "GLFW/glfw3.h"
#include "RxECS.h"
#include "Input/Keyboard.hpp"
#include "Modules/Module.h"

namespace RxEngine
{
    struct MousePosition
    {
        float x;
        float y;

        int32_t mods;
    };

    struct MouseButton
    {
        uint32_t button;
        bool pressed;
        int32_t mods;
    };

    struct MouseScroll
    {
        float y_offset;
        int32_t mods;
    };

    class MouseModule : public Module
    {
        //friend void callbackCursorPos(GLFWwindow* /*window*/, double xpos, double ypos);
        //friend void callbackMouseButton(GLFWwindow* /*window */, int32_t button, int32_t action, int32_t /*mods*/);
        //friend void callbackScroll(GLFWwindow* /*window*/, double /*xoffset*/, double y_offset);

    public:
        void registerModule() override;
        void startup() override;
        void shutdown() override;
        void deregisterModule() override;

        void onMove(float xpos, float yPos, EInputMod mods);
        void onButton(int32_t button, bool pressed, EInputMod mods);
        void onScroll(float yScroll, EInputMod mods);
    };
}
