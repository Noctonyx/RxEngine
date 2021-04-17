#include "Window.hpp"
#include "ini.h"

namespace RxEngine
{
    void FrameBufferResizeCallback(GLFWwindow * window, int width, int height)
    {
        Window * w = static_cast<Window *>(glfwGetWindowUserPointer(window));
        w->onResize.Broadcast(width, height);
    }

    Window::Window(uint32_t width, uint32_t height, const std::string & title)
    {
        if (!m_GLFWInit) {
            glfwInit();
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            m_GLFWInit = true;
        }

        mINI::INIFile file("config.ini");
        mINI::INIStructure ini;

        file.read(ini);
        auto w = ini.get("window").get("width");

        m_Window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(m_Window, this);

        glfwSetFramebufferSizeCallback(m_Window, &FrameBufferResizeCallback);
    }

    Window::~Window()
    {
        mouse.reset();
        keyboard.reset();

        glfwDestroyWindow(m_Window);
        glfwTerminate();
    }

    uint32_t Window::GetWidth() const
    {
        int height, width;
        glfwGetFramebufferSize(m_Window, &width, &height);
        return width;
    }

    uint32_t Window::GetHeight() const
    {
        int height, width;
        glfwGetFramebufferSize(m_Window, &width, &height);
        return height;
    }

    float Window::GetAspectRatio() const
    {
        int height, width;
        glfwGetFramebufferSize(m_Window, &width, &height);
        return float(width) / float(height);
    }

    void Window::SetMouseVisible(bool visible)
    {
        m_IsMouseVisible = visible;
        if (visible) { glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); }
        else { glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); }
    }

    bool Window::GetMouseVisible() const { return m_IsMouseVisible; }

    void Window::SetTitle(std::string & title) { glfwSetWindowTitle(m_Window, title.c_str()); }

    GLFWwindow * Window::GetWindow() const { return m_Window; }
} // namespace RXCore
