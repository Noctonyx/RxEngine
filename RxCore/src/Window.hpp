#pragma once

#include <string>
#include <string>
#include "SDL_mouse.h"
#include "SDL_video.h"

namespace RxCore
{
    enum class ECursorStandard : uint32_t
    {
        None,
        Arrow,
        IBeam,
        Hand,
        ResizeX,
        ResizeY
    };

    class Window
    {
    public:
        Window(uint32_t width, uint32_t height, const std::string & title);

        ~Window();

        Window(const Window & other) = delete;

        Window(Window && other) noexcept = delete;

        Window & operator=(const Window & other) = delete;

        Window & operator=(Window && other) noexcept = delete;

        [[nodiscard]] uint32_t getWidth() const;

        [[nodiscard]] uint32_t getHeight() const;
        void setRelativeMouseMode(bool mode);

        [[nodiscard]] bool getMouseVisible() const;

        void setTitle(std::string & title);

        [[nodiscard]] SDL_Window * GetWindow() const;

        void setCursor(ECursorStandard standard) const;
        void hideCursor(bool hidden);
        void setCursorPosition(int32_t x, int32_t y);

    private:
        SDL_Window * m_Window = nullptr;
        bool m_IsMouseVisible = true;
        float cursorX;
        float cursorY;

        SDL_Cursor* cursorArrow;
        SDL_Cursor* cursorIBeam;
        SDL_Cursor* cursorResizeY;
        SDL_Cursor* cursorResizeX;
        SDL_Cursor* cursorHand;

        bool hidden_ = false;
    };
} // namespace RXCore
