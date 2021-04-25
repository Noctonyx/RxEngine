#include "Window.hpp"
#include "ini.h"
#include "RxECS.h"
#include "Input/Keyboard.hpp"

namespace RxEngine
{
    void callbackKey(GLFWwindow* window,
        int32_t key,
        int32_t scancode,
        int32_t action,
        int32_t mods)
    {
        auto wnd = static_cast<Window*>(glfwGetWindowUserPointer(window));
        wnd->doKey(key, action, mods);
    }

    void callbackChar(GLFWwindow* window, uint32_t codepoint)
    {
        auto wnd = static_cast<Window*>(glfwGetWindowUserPointer(window));
        wnd->doChar(codepoint);
    }

    void callbackCursorPos(GLFWwindow* window, double xpos, double ypos)
    {
        auto wnd = static_cast<Window*>(glfwGetWindowUserPointer(window));
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

        wnd->mousePosition(static_cast<float>(xpos), static_cast<float>(ypos), mods);
    }

    void callbackMouseButton(GLFWwindow* window, int32_t button, int32_t action, int32_t mods)
    {
        auto wnd = static_cast<Window*>(glfwGetWindowUserPointer(window));

        wnd->buttonAction(button, action == GLFW_PRESS, mods);
    }

    void callbackScroll(GLFWwindow* window, double /*xoffset*/, double y_offset)
    {
        auto wnd = static_cast<Window*>(glfwGetWindowUserPointer(window));
       
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

        wnd->scroll(static_cast<float>(y_offset), mods);
    }

    void FrameBufferResizeCallback(GLFWwindow * window, int width, int height)
    {
        Window * w = static_cast<Window *>(glfwGetWindowUserPointer(window));
        w->doResize(width, height);
    }

    Window::Window(uint32_t width, uint32_t height, const std::string & title, ecs::World * world)
        : world_(world)
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

        glfwSetCursorPosCallback(GetWindow(), callbackCursorPos);
        glfwSetMouseButtonCallback(GetWindow(), callbackMouseButton);
        glfwSetScrollCallback(GetWindow(), callbackScroll);

        glfwSetKeyCallback(GetWindow(), callbackKey);
        glfwSetCharCallback(GetWindow(), callbackChar);

        world->setSingleton<WindowDetails>({ .width = width, .height = height });
    }

    Window::~Window()
    {
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
        if (visible) {
            glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    bool Window::GetMouseVisible() const
    {
        return m_IsMouseVisible;
    }

    void Window::SetTitle(std::string & title)
    {
        glfwSetWindowTitle(m_Window, title.c_str());
    }

    GLFWwindow * Window::GetWindow() const
    {
        return m_Window;
    }

    void Window::doResize(uint32_t width, uint32_t height)
    {
        onResize.Broadcast(width, height);
        world_->getStream<WindowResize>()->add<WindowResize>({width, height});
    }

    void Window::doChar(uint32_t codepoint)
    {
        //onChar.Broadcast(static_cast<char>(codepoint));

        auto s = world_->getStream<KeyboardChar>();
        s->add<KeyboardChar>({ static_cast<char>(codepoint) });
    }

    void Window::doKey(int32_t key, int32_t action, int32_t mods)
    {
        //  onKey.Broadcast(static_cast<EKey>(key), static_cast<EInputAction>(action),
        //                  static_cast<EInputMod>(mods));

        auto s = world_->getStream<KeyboardKey>();

        s->add<KeyboardKey>({
            static_cast<EKey>(key), static_cast<EInputAction>(action),
            static_cast<EInputMod>(mods)
            });
    }


    void Window::mousePosition(float x_pos, float y_pos, int32_t mods)
    {
        pos_ = { x_pos, y_pos };
        //onMousePos.Broadcast(x_pos, y_pos, static_cast<RxEngine::EInputMod>(mods));

        world_->getStream<MousePosition>()->add<MousePosition>({
            x_pos, y_pos, static_cast<RxEngine::EInputMod>(mods)
            });
    }

    void Window::buttonAction(int32_t button, bool pressed, int32_t mods)
    {
        //onMouseButton.Broadcast(button, pressed, static_cast<RxEngine::EInputMod>(mods));

        world_->getStream<MouseButton>()->add<MouseButton>({
            button, pressed, static_cast<RxEngine::EInputMod>(mods)
            });
    }

    void Window::scroll(float y_scroll, int32_t mods)
    {
        //onScroll.Broadcast(y_scroll, static_cast<RxEngine::EInputMod>(mods));
        world_->getStream<MouseScroll>()->add<MouseScroll>({
            y_scroll, static_cast<RxEngine::EInputMod>(mods)
            });
    }

    void Window::setCursor(ECursorStandard standard)
    {
        //auto window = Locator::Window::get();
        if (standard == cursorStandard_) {
            return;
        }

        cursor_ = glfwCreateStandardCursor(static_cast<int32_t>(standard));
        glfwSetCursor(GetWindow(), cursor_);
        cursorStandard_ = standard;
    }

    void Window::hideCursor(bool hidden)
    {
        if (hidden_ == hidden) {
            return;
        }

        glfwSetInputMode(
            GetWindow(),
            GLFW_CURSOR,
            hidden ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        if (!hidden) {
            setPosition(DirectX::XMFLOAT2{ pos_.x, pos_.y });
        }
        hidden_ = hidden;
    }

    void Window::setPosition(const DirectX::XMFLOAT2& position)
    {
        pos_ = position;
        glfwSetCursorPos(GetWindow(), pos_.x, pos_.y);
    }
} // namespace RXCore
