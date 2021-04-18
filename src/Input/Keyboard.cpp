//
// Copyright (c) 2020 - Shane Hyde (shane@noctonyx.com)
//

#include <GLFW/glfw3.h>
#include "Keyboard.hpp"
#include "Window.hpp"
#include <cassert>
#include "RxECS.h"
//#include "EnumClass.h"

namespace RxEngine
{
    void callbackKey(GLFWwindow * window,
                     int32_t key,
                     int32_t scancode,
                     int32_t action,
                     int32_t mods)
    {
        auto wnd = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
        assert(wnd->keyboard);
        wnd->keyboard->doKey(key, action, mods);
    }

    void callbackChar(GLFWwindow * window, uint32_t codepoint)
    {
        auto wnd = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
        assert(wnd->keyboard);
        wnd->keyboard->doChar(codepoint);
    }

    Keyboard::Keyboard(Window * window, ecs::World * world)
        : window_(window)
        , world_(world)
    {
        glfwSetKeyCallback(window_->GetWindow(), callbackKey);
        glfwSetCharCallback(window_->GetWindow(), callbackChar);
    }

    void Keyboard::doChar(uint32_t codepoint)
    {
        //onChar.Broadcast(static_cast<char>(codepoint));

        auto s = world_->getStream<KeyboardChar>();
        s->add<KeyboardChar>({static_cast<char>(codepoint)});
    }

    void Keyboard::doKey(int32_t key, int32_t action, int32_t mods)
    {
      //  onKey.Broadcast(static_cast<EKey>(key), static_cast<EInputAction>(action),
      //                  static_cast<EInputMod>(mods));

        auto s = world_->getStream<KeyboardKey>();

        s->add<KeyboardKey>({
            static_cast<EKey>(key), static_cast<EInputAction>(action),
            static_cast<EInputMod>(mods)
        });
    }
}
