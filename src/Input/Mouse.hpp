//
// Copyright (c) 2020 - Shane Hyde (shane@noctonyx.com)
//

#ifndef AMX_MOUSE_HPP
#define AMX_MOUSE_HPP

#include <GLFW/glfw3.h>
#include "DirectXMath.h"
#include "Delegates.hpp"
#include "Keyboard.hpp"

namespace RxEngine
{
    class Window;

    enum class ECursorHotspot : uint8_t
    {
        UpperLeft,
        UpperRight,
        BottomLeft,
        BottomRight,
        Centered
    };

    enum class ECursorStandard : uint32_t
    {
        Arrow = 0x00036001,
        IBeam = 0x00036002,
        Crosshair = 0x00036003,
        Hand = 0x00036004,
        ResizeX = 0x00036005,
        ResizeY = 0x00036006
    };

    struct MousePosition
    {
        float x;
        float y;

        EInputMod mods;
    };

    struct MouseButton
    {
        int32_t button;
        bool pressed;
        EInputMod mods;
    };

    struct MouseScroll
    {
        float y_offset;
        EInputMod mods;
    };
#if 0
    class Mouse
    {
        friend void callbackCursorPos(GLFWwindow* /*window*/, double xpos, double ypos);
        friend void callbackMouseButton(GLFWwindow* /*window */, int32_t button, int32_t action, int32_t /*mods*/);
        friend void callbackScroll(GLFWwindow* /*window*/, double /*xoffset*/, double y_offset);

    public:
        explicit Mouse(Window* window, ecs::World* world);

        void setCursor(ECursorStandard standard);
        void hideCursor(bool hidden);
        void setPosition(const DirectX::XMFLOAT2& position);

        [[nodiscard]] const DirectX::XMFLOAT2& getPosition() const { return pos_; }

        //MulticastDelegate<float, float, RxEngine::EInputMod> onMousePos;
        //MulticastDelegate<int32_t, bool, RxEngine::EInputMod> onMouseButton;
        //MulticastDelegate<float, RxEngine::EInputMod> onScroll;

    private:
        Window* window_;
        ecs::World* world_;
        DirectX::XMFLOAT2 pos_;
        GLFWcursor* cursor_;
        ECursorStandard cursorStandard_;
        bool hidden_ = false;

        void mousePosition(float x_pos, float y_pos, int32_t mods);
        void buttonAction(int32_t button, bool pressed, int32_t mods);
        void scroll(float y_scroll, int32_t mods);
    };
#endif
}
#endif //AMX_MOUSE_HPP
