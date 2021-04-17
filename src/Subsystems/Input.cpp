#include "Input.h"

#include "imgui.h"
#include "Input/Keyboard.hpp"

namespace RxEngine
{
#if 0
    void Input::OnMouseButton(int32_t button, bool pressed)
    {
        auto & io = ImGui::GetIO();
        if (io.WantCaptureMouse) {
            return;
        }

        onMouseButton.Broadcast(button, pressed);
    }

    void Input::OnMousePos(float x, float y)
    {
        auto & io = ImGui::GetIO();
        if (io.WantCaptureMouse) {
            return;
        }

        onMousePos.Broadcast(x, y);
    }

    void Input::OnMouseScroll(float yscroll)
    {
        auto & io = ImGui::GetIO();
        if (io.WantCaptureMouse) {
            return;
        }

        onMouseScroll.Broadcast(yscroll);
    }

    void Input::OnKey(RXEngine::EKey key, RXEngine::EInputAction action, RXEngine::EInputMod mods)
    {
        auto & io = ImGui::GetIO();
        if (io.WantCaptureKeyboard) {
            return;
        }

        onKeyboardKey.Broadcast(key, action, mods);
    }

    void Input::OnChar(char c)
    {
        auto & io = ImGui::GetIO();
        if (io.WantCaptureKeyboard) {
            return;
        }

        onKeyboardChar.Broadcast(c);
    }

    void Input::addedToScene(Scene * scene, std::vector<std::shared_ptr<Subsystem>> & subsystems)
    {
        hOnMouseButton = mouse_->onMouseButton.AddRaw(this, &Input::OnMouseButton);
        hOnMousePos = mouse_->onMousePos.AddRaw(this, &Input::OnMousePos);
        hOnMouseScroll = mouse_->onScroll.AddRaw(this, &Input::OnMouseScroll);

        hOnKey = keyboard_->onKey.AddRaw(this, &Input::OnKey);
        hOnChar = keyboard_->onChar.AddRaw(this, &Input::OnChar);
    }

    void Input::removedFromScene()
    {
        mouse_->onMouseButton.Remove(hOnMouseButton);
        mouse_->onMousePos.Remove(hOnMousePos);
        mouse_->onScroll.Remove(hOnMouseScroll);
        keyboard_->onChar.Remove(hOnChar);
        keyboard_->onKey.Remove(hOnKey);
    }
#endif
}
