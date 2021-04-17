#pragma once

#include "Subsystem.h"
#include "Input/Mouse.hpp"

namespace RxEngine
{
#if 0
    enum class EKey : int16_t;
    enum class EInputAction;
    enum class EInputMod;
    class DelegateHandle;
    class Mouse;
    class Keyboard;

    class Input : public Subsystem
    {
    public:
        Input(
            const std::shared_ptr<Mouse> & mouse,
            const std::shared_ptr<Keyboard> & keyboard)
            : mouse_(mouse)
            , keyboard_(keyboard) {}

        void OnMouseButton(int32_t button, bool pressed);
        void OnMousePos(float x, float y);
        void OnMouseScroll(float yscroll);
        void OnKey(EKey key, EInputAction action, EInputMod mods);
        void OnChar(char c);

        void addedToScene(Scene * scene,
                          std::vector<std::shared_ptr<Subsystem>> & subsystems) override;
        void removedFromScene() override;

        MulticastDelegate<float, float> onMousePos;
        MulticastDelegate<int32_t, bool> onMouseButton;
        MulticastDelegate<float> onMouseScroll;
        MulticastDelegate<EKey, EInputAction, EInputMod>
        onKeyboardKey;
        MulticastDelegate<char> onKeyboardChar;

    private:

        DelegateHandle hOnMouseButton;
        DelegateHandle hOnMousePos;
        DelegateHandle hOnMouseScroll;
        DelegateHandle hOnKey;
        DelegateHandle hOnChar;

        std::shared_ptr<Mouse> mouse_;
        std::shared_ptr<Keyboard> keyboard_;
    };
#endif
}
