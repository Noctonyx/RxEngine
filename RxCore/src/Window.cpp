#include "Window.hpp"
#include "DirectXMath.h"
#include "Log.h"
#include "SDL.h"
#include "SDL_vulkan.h"

namespace RxCore
{
    Window::Window(uint32_t width, uint32_t height, const std::string & title)
        : cursorX(0)
        , cursorY(0)
    {
        //SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
        m_Window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                    static_cast<int>(width), static_cast<int>(height),
                                    SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI |
                                    SDL_WINDOW_RESIZABLE);


        cursorArrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
        cursorResizeX = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
        cursorResizeY = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
        cursorIBeam = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
        //cursorEggtimer = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
        cursorHand = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    }

    Window::~Window()
    {
        SDL_DestroyWindow(m_Window);

        SDL_FreeCursor(cursorArrow);
        SDL_FreeCursor(cursorHand);
        SDL_FreeCursor(cursorResizeX);
        SDL_FreeCursor(cursorResizeY);
        SDL_FreeCursor(cursorIBeam);
    }

    uint32_t Window::getWidth() const
    {
        int height, width;
        SDL_Vulkan_GetDrawableSize(m_Window, &width, &height);
        return width;
    }

    uint32_t Window::getHeight() const
    {
        int height, width;
        SDL_Vulkan_GetDrawableSize(m_Window, &width, &height);
        return height;
    }

    void Window::setRelativeMouseMode(bool mode)
    {
        SDL_SetRelativeMouseMode(mode ? SDL_TRUE : SDL_FALSE);
    }

    bool Window::getMouseVisible() const
    {
        return m_IsMouseVisible;
    }

    void Window::setTitle(std::string & title)
    {
        SDL_SetWindowTitle(m_Window, title.c_str());
    }

    SDL_Window * Window::GetWindow() const
    {
        return m_Window;
    }

    void Window::setCursor(ECursorStandard standard) const
    {
        switch (standard) {
        case ECursorStandard::None:
            SDL_ShowCursor(SDL_FALSE);
            break;
        case ECursorStandard::Arrow:
            SDL_ShowCursor(SDL_TRUE);
            SDL_SetCursor(cursorArrow);
            break;
        case ECursorStandard::IBeam:
            SDL_ShowCursor(SDL_TRUE);
            SDL_SetCursor(cursorIBeam);
            break;
        case ECursorStandard::ResizeX:
            SDL_ShowCursor(SDL_TRUE);
            SDL_SetCursor(cursorResizeX);
            break;
        case ECursorStandard::ResizeY:
            SDL_ShowCursor(SDL_TRUE);
            SDL_SetCursor(cursorResizeY);
            break;
        case ECursorStandard::Hand:
            SDL_ShowCursor(SDL_TRUE);
            SDL_SetCursor(cursorHand);
            break;
        }
    }

    void Window::hideCursor(bool hidden)
    {
        if (hidden_ == hidden) {
            return;
        }

        SDL_ShowCursor(hidden ? SDL_DISABLE : SDL_ENABLE);
        SDL_SetRelativeMouseMode(hidden ? SDL_TRUE : SDL_FALSE);

        if (!hidden) {
            setCursorPosition(static_cast<int32_t>(cursorX), static_cast<int32_t>(cursorY));
        } else {
            int x, y;
            SDL_GetMouseState(&x, &y);
            cursorX = static_cast<float>(x);
            cursorY = static_cast<float>(y);
        }
        hidden_ = hidden;
    }

    void Window::setCursorPosition(int32_t x, int32_t y)
    {
        cursorX = static_cast<float>(x);
        cursorY = static_cast<float>(y);

        SDL_WarpMouseInWindow(m_Window, x, y);
    }
} // namespace RXCore
