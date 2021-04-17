#pragma once

#include <string>
#include <memory>
#include <string>
#include <GLFW/glfw3.h>

#include "Delegates.hpp"

namespace RxEngine
{
    class Keyboard;
    class Mouse;

    class Window
    {
    public:
        Window(uint32_t width, uint32_t height, const std::string & title);

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

        std::shared_ptr<Mouse> mouse;
        std::shared_ptr<Keyboard> keyboard;

        MulticastDelegate<int, int> onResize;

    private:
        bool m_GLFWInit = false;
        GLFWwindow * m_Window = nullptr;
        bool m_IsMouseVisible = true;
    };
} // namespace RXCore
