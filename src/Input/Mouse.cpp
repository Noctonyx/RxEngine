//
// Copyright (c) 2020 - Shane Hyde (shane@noctonyx.com)
//

#include <cassert>
#include "GLFW/glfw3.h"
#include "DirectXMath.h"
//#include "glm/vec2.hpp"
#include "Window.hpp"
#include "Mouse.hpp"
#include "RxECS.h"
#include "Stream.h"
#include "World.h"

namespace RxEngine
{
    void callbackCursorPos(GLFWwindow * window, double xpos, double ypos)
    {
        auto wnd = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
        assert(wnd->mouse);

        int32_t mods = 0;

        if (glfwGetKey(window, static_cast<int>(EKey::ShiftLeft)) == GLFW_PRESS) {
            mods |= GLFW_MOD_SHIFT;
        }
        if (glfwGetKey(window, static_cast<int>(EKey::ShiftRight)) == GLFW_PRESS) {
            mods |= GLFW_MOD_SHIFT;
        }
        if (glfwGetKey(window, static_cast<int>(EKey::ControlLeft)) == GLFW_PRESS) {
            mods |= GLFW_MOD_CONTROL;
        }
        if (glfwGetKey(window, static_cast<int>(EKey::ControlRight)) == GLFW_PRESS) {
            mods |= GLFW_MOD_CONTROL;
        }
        if (glfwGetKey(window, static_cast<int>(EKey::AltLeft)) == GLFW_PRESS) {
            mods |= GLFW_MOD_ALT;
        }
        if (glfwGetKey(window, static_cast<int>(EKey::AltRight)) == GLFW_PRESS) {
            mods |= GLFW_MOD_ALT;
        }

        wnd->mouse->mousePosition(static_cast<float>(xpos), static_cast<float>(ypos), mods);
    }

    void callbackMouseButton(GLFWwindow * window, int32_t button, int32_t action, int32_t mods)
    {
        auto wnd = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
        assert(wnd->mouse);

        wnd->mouse->buttonAction(button, action == GLFW_PRESS, mods);
    }

    void callbackScroll(GLFWwindow * window, double /*xoffset*/, double y_offset)
    {
        auto wnd = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
        assert(wnd->mouse);

        int32_t mods = 0;

        if (glfwGetKey(window, static_cast<int>(EKey::ShiftLeft)) == GLFW_PRESS) {
            mods |= GLFW_MOD_SHIFT;
        }
        if (glfwGetKey(window, static_cast<int>(EKey::ShiftRight)) == GLFW_PRESS) {
            mods |= GLFW_MOD_SHIFT;
        }
        if (glfwGetKey(window, static_cast<int>(EKey::ControlLeft)) == GLFW_PRESS) {
            mods |= GLFW_MOD_CONTROL;
        }
        if (glfwGetKey(window, static_cast<int>(EKey::ControlRight)) == GLFW_PRESS) {
            mods |= GLFW_MOD_CONTROL;
        }
        if (glfwGetKey(window, static_cast<int>(EKey::AltLeft)) == GLFW_PRESS) {
            mods |= GLFW_MOD_ALT;
        }
        if (glfwGetKey(window, static_cast<int>(EKey::AltRight)) == GLFW_PRESS) {
            mods |= GLFW_MOD_ALT;
        }

        wnd->mouse->scroll(static_cast<float>(y_offset), mods);
    }

    Mouse::Mouse(Window * window, ecs::World * world)
        : window_(window)
        , world_(world)
        , pos_(0, 0)
    {
        glfwSetCursorPosCallback(window_->GetWindow(), callbackCursorPos);
        glfwSetMouseButtonCallback(window_->GetWindow(), callbackMouseButton);
        glfwSetScrollCallback(window_->GetWindow(), callbackScroll);
    }

    void Mouse::mousePosition(float x_pos, float y_pos, int32_t mods)
    {
        pos_ = {x_pos, y_pos};
        //onMousePos.Broadcast(x_pos, y_pos, static_cast<RxEngine::EInputMod>(mods));

        world_->getStream<MousePosition>()->add<MousePosition>({
            x_pos, y_pos, static_cast<RxEngine::EInputMod>(mods)
        });
    }

    void Mouse::buttonAction(int32_t button, bool pressed, int32_t mods)
    {
        //onMouseButton.Broadcast(button, pressed, static_cast<RxEngine::EInputMod>(mods));

        world_->getStream<MouseButton>()->add<MouseButton>({
            button, pressed, static_cast<RxEngine::EInputMod>(mods)
        });
    }

    void Mouse::scroll(float y_scroll, int32_t mods)
    {
        //onScroll.Broadcast(y_scroll, static_cast<RxEngine::EInputMod>(mods));
        world_->getStream<MouseScroll>()->add<MouseScroll>({
            y_scroll, static_cast<RxEngine::EInputMod>(mods)
        });
    }

    void Mouse::setCursor(ECursorStandard standard)
    {
        //auto window = Locator::Window::get();
        if (standard == cursorStandard_) {
            return;
        }

        cursor_ = glfwCreateStandardCursor(static_cast<int32_t>(standard));
        glfwSetCursor(window_->GetWindow(), cursor_);
        cursorStandard_ = standard;
    }

    void Mouse::hideCursor(bool hidden)
    {
        if (hidden_ == hidden) {
            return;
        }

        glfwSetInputMode(
            window_->GetWindow(),
            GLFW_CURSOR,
            hidden ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        if (!hidden) {
            setPosition(DirectX::XMFLOAT2{pos_.x, pos_.y});
        }
        hidden_ = hidden;
    }

    void Mouse::setPosition(const DirectX::XMFLOAT2 & position)
    {
        pos_ = position;
        glfwSetCursorPos(window_->GetWindow(), pos_.x, pos_.y);
    }
}
