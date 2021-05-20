#include "Window.hpp"
#include <GLFW/glfw3.h>

#include "DirectXMath.h"
#include "EngineMain.hpp"
#include "ini.h"
#include "RxECS.h"
#include "Log.h"

namespace RxEngine
{
    void callbackKey(GLFWwindow * window,
                     int32_t key,
                     int32_t scancode,
                     int32_t action,
                     int32_t mods)
    {
        auto wnd = static_cast<Window *>(glfwGetWindowUserPointer(window));
        wnd->doKey(key, action, mods);
    }

    void callbackChar(GLFWwindow * window, uint32_t codepoint)
    {
        auto wnd = static_cast<Window *>(glfwGetWindowUserPointer(window));
        wnd->doChar(codepoint);
    }

    void callbackCursorPos(GLFWwindow * window, double xpos, double ypos)
    {
        auto wnd = static_cast<Window *>(glfwGetWindowUserPointer(window));
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

    void callbackMouseButton(GLFWwindow * window, int32_t button, int32_t action, int32_t mods)
    {
        auto wnd = static_cast<Window *>(glfwGetWindowUserPointer(window));

        spdlog::info("Mouse Button = {}, pressed = {}", button, action);
        wnd->buttonAction(button, action == GLFW_PRESS, mods);
    }

    void callbackScroll(GLFWwindow * window, double /*xoffset*/, double y_offset)
    {
        auto wnd = static_cast<Window *>(glfwGetWindowUserPointer(window));

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

        glfwSetCursorPosCallback(GetWindow(), callbackCursorPos);
        glfwSetMouseButtonCallback(GetWindow(), callbackMouseButton);
        glfwSetScrollCallback(GetWindow(), callbackScroll);

        glfwSetKeyCallback(GetWindow(), callbackKey);
        glfwSetCharCallback(GetWindow(), callbackChar);
    }

    Window::~Window()
    {
        glfwDestroyWindow(m_Window);
        glfwTerminate();
    }

    void Window::Update()
    {
        glfwPollEvents();
    }

    uint32_t Window::getWidth() const
    {
        int height, width;
        glfwGetFramebufferSize(m_Window, &width, &height);
        return width;
    }

    uint32_t Window::getHeight() const
    {
        int height, width;
        glfwGetFramebufferSize(m_Window, &width, &height);
        return height;
    }

    float Window::getAspectRatio() const
    {
        int height, width;
        glfwGetFramebufferSize(m_Window, &width, &height);
        return float(width) / float(height);
    }

    bool Window::shouldClose() const
    {
        return glfwWindowShouldClose(m_Window);
    }

    void Window::setMouseVisible(bool visible)
    {
        m_IsMouseVisible = visible;
        if (visible) {
            glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    bool Window::getMouseVisible() const
    {
        return m_IsMouseVisible;
    }

    void Window::setTitle(std::string & title)
    {
        glfwSetWindowTitle(m_Window, title.c_str());
    }

    GLFWwindow * Window::GetWindow() const
    {
        return m_Window;
    }

    void Window::doResize(uint32_t width, uint32_t height)
    {
        //onResize.Broadcast(width, height);
        if (world_) {
            world_->getStream<WindowResize>()->add<WindowResize>({width, height});
            world_->setSingleton<WindowDetails>({this, width, height});
        }
    }

    void Window::doChar(uint32_t codepoint)
    {
        //onChar.Broadcast(static_cast<char>(codepoint));

        if (world_) {
            auto s = world_->getStream<KeyboardChar>();
            s->add<KeyboardChar>({static_cast<char>(codepoint)});
        }
    }

    void Window::doKey(int32_t key, int32_t action, int32_t mods)
    {
        //  onKey.Broadcast(static_cast<EKey>(key), static_cast<EInputAction>(action),
        //                  static_cast<EInputMod>(mods));

        if (world_) {
            auto s = world_->getStream<KeyboardKey>();

            s->add<KeyboardKey>({
                static_cast<EKey>(key), static_cast<EInputAction>(action),
                static_cast<EInputMod>(mods)
            });
        }
    }


    void Window::mousePosition(float x_pos, float y_pos, int32_t mods)
    {
        cursorX = x_pos;
        cursorY = y_pos;

        if (world_) {
            world_->getStream<MousePosition>()->add<MousePosition>({
                x_pos, y_pos, static_cast<RxEngine::EInputMod>(mods)
            });

            auto mouse_status = world_->getSingletonUpdate<MouseStatus>();

            float delta = world_->deltaTime();

            auto deltaMouseX = (cursorX - mouse_status->mouseX) / delta / 50.0f / 180.f *
                DirectX::XM_2PI;
            auto deltaMouseY = (cursorY - mouse_status->mouseY) / delta / 50.0f / 180.f *
                DirectX::XM_2PI;

            mouse_status->mouseX = cursorX;
            mouse_status->mouseY = cursorY;

            mouse_status->deltaMouseX = deltaMouseX;
            mouse_status->deltaMouseY = deltaMouseY;
        }
    }

    void Window::buttonAction(int32_t button, bool pressed, int32_t mods)
    {
        if (world_) {
            world_->getStream<MouseButton>()->add<MouseButton>({
                button, pressed, static_cast<RxEngine::EInputMod>(mods)
            });

            auto mbs = world_->getSingletonUpdate<MouseStatus>();
            switch (button) {
            case 0:
                mbs->button1 = pressed;
                break;
            case 1:
                mbs->button2 = pressed;
                break;
            case 2:
                mbs->button3 = pressed;
                break;
            default:
                break;
            }
        }
    }

    void Window::scroll(float y_scroll, int32_t mods)
    {
        if (world_) {
            world_->getStream<MouseScroll>()->add<MouseScroll>({
                y_scroll, static_cast<RxEngine::EInputMod>(mods)
            });
        }
    }

    void mouseStatusUi(ecs::World *, void * ptr)
    {
        auto ms = static_cast<MouseStatus *>(ptr);
        if (ms) {

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("X Position");
            ImGui::TableNextColumn();
            ImGui::Text("%f", ms->mouseX);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Y Position");
            ImGui::TableNextColumn();
            ImGui::Text("%f", ms->mouseY);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Delta X");
            ImGui::TableNextColumn();
            ImGui::Text("%f", ms->deltaMouseX);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Delta Y");
            ImGui::TableNextColumn();
            ImGui::Text("%f", ms->deltaMouseY);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Button 1");
            ImGui::TableNextColumn();
            ImGui::Text("%s", ms->button1 ? "Pressed" : "Released");
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Button 2");
            ImGui::TableNextColumn();
            ImGui::Text("%s", ms->button2 ? "Pressed" : "Released");
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Button 3");
            ImGui::TableNextColumn();
            ImGui::Text("%s", ms->button3 ? "Pressed" : "Released");
        }
    }

    void Window::setWorld(ecs::World * world)
    {
        int w, h;
        world_ = world;
        glfwGetWindowSize(m_Window, &w, &h);
        world->setSingleton<WindowDetails>({
            .width = static_cast<uint32_t>(w),
            .height = static_cast<uint32_t>(h)
        });

        world->setSingleton<MouseStatus>({
            .button1 = false,
            .button2 = false,
            .button3 = false
        });

        world->set<ComponentGui>(world->getComponentId<MouseStatus>(), {.editor = mouseStatusUi});

        world->createSystem("Window:ResetDeltas")
            .inGroup("Pipeline:PostFrame")
            .withWrite<MouseStatus>()
            .execute([](ecs::World* w)
                {
                    auto ms = w->getSingletonUpdate<MouseStatus>();
                    ms->deltaMouseX = 0.f;
                    ms->deltaMouseY = 0.f;
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
            setCursorPosition(static_cast<int32_t>(cursorX), static_cast<int32_t>(cursorY)); // DirectX::XMFLOAT2{ pos_.x, pos_.y });
        }
        hidden_ = hidden;
    }

    void Window::setCursorPosition(int32_t x, int32_t y)
    {
        cursorX = static_cast<float>(x);
        cursorY = static_cast<float>(y);

        glfwSetCursorPos(GetWindow(), cursorX, cursorY);
    }
} // namespace RXCore
