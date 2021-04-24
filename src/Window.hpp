#pragma once

#include <string>
#include <memory>
#include <string>
#include <GLFW/glfw3.h>

#include "Delegates.hpp"
#include "DirectXMath.h"
#include "Input/Mouse.hpp"
#include "RxECS.h"

namespace ecs {
    class World;
}

namespace RxEngine
{
    class Window;
    class Keyboard;
    class Mouse;

    struct WindowDetails
    {
        Window* window;

        uint32_t width;
        uint32_t height;
    };

    struct WindowResize
    {
        uint32_t width;
        uint32_t height;
    };

    class Window
    {
    public:
        Window(uint32_t width, uint32_t height, const std::string & title, ecs::World * world);

        ~Window();

        Window(const Window & other) = delete;

        Window(Window && other) noexcept = delete;

        Window & operator=(const Window & other) = delete;

        Window & operator=(Window && other) noexcept = delete;

        void Update() { glfwPollEvents(); }

        [[nodiscard]] uint32_t GetWidth() const;

        [[nodiscard]] uint32_t GetHeight() const;

        [[nodiscard]] float GetAspectRatio() const;

        [[nodiscard]] bool ShouldClose() const { return glfwWindowShouldClose(m_Window); }

        void SetMouseVisible(bool visible);

        [[nodiscard]] bool GetMouseVisible() const;

        void SetTitle(std::string & title);

        [[nodiscard]] GLFWwindow * GetWindow() const;

        //std::shared_ptr<Mouse> mouse;
        //std::shared_ptr<Keyboard> keyboard;

        MulticastDelegate<int, int> onResize;
     

        void setCursor(ECursorStandard standard);
        void hideCursor(bool hidden);
        void setPosition(const DirectX::XMFLOAT2& position);

        [[nodiscard]] const DirectX::XMFLOAT2& getPosition() const { return pos_; }

        void doResize(uint32_t width, uint32_t height);
        void doChar(uint32_t codepoint);
        void doKey(int32_t key, int32_t action, int32_t mods);
        void mousePosition(float x_pos, float y_pos, int32_t mods);
        void buttonAction(int32_t button, bool pressed, int32_t mods);
        void scroll(float y_scroll, int32_t mods);
        
    private:
        bool m_GLFWInit = false;
        GLFWwindow * m_Window = nullptr;
        bool m_IsMouseVisible = true;
        ecs::World* world_ = nullptr;
        DirectX::XMFLOAT2 pos_{};
        GLFWcursor* cursor_ = nullptr;
        ECursorStandard cursorStandard_{};
        bool hidden_ = false;
    };
} // namespace RXCore
