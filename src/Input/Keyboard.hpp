//
// Copyright (c) 2020 - Shane Hyde (shane@noctonyx.com)
//

#ifndef AMX_KEYBOARD_HPP
#define AMX_KEYBOARD_HPP

#include "Delegates.hpp"

namespace RxEngine
{
    class Window;

    enum class EKey : int16_t
    {
        Unknown = -1,
        Space = 32,
        Apostrophe = 39,
        Comma = 44,
        Minus = 45,
        Period = 46,
        Slash = 47,
        _0 = 48,
        _1 = 49,
        _2 = 50,
        _3 = 51,
        _4 = 52,
        _5 = 53,
        _6 = 54,
        _7 = 55,
        _8 = 56,
        _9 = 57,
        Semicolon = 59,
        Equal = 61,
        A = 65,
        B = 66,
        C = 67,
        D = 68,
        E = 69,
        F = 70,
        G = 71,
        H = 72,
        I = 73,
        J = 74,
        K = 75,
        L = 76,
        M = 77,
        N = 78,
        O = 79,
        P = 80,
        Q = 81,
        R = 82,
        S = 83,
        T = 84,
        U = 85,
        V = 86,
        W = 87,
        X = 88,
        Y = 89,
        Z = 90,
        LeftBracket = 91,
        Backslash = 92,
        RightBracket = 93,
        GraveAccent = 96,
        World1 = 161,
        World2 = 162,
        Escape = 256,
        Enter = 257,
        Tab = 258,
        Backspace = 259,
        Insert = 260,
        Delete = 261,
        Right = 262,
        Left = 263,
        Down = 264,
        Up = 265,
        PageUp = 266,
        PageDown = 267,
        Home = 268,
        End = 269,
        CapsLock = 280,
        ScrollLock = 281,
        NumLock = 282,
        PrintScreen = 283,
        Pause = 284,
        F1 = 290,
        F2 = 291,
        F3 = 292,
        F4 = 293,
        F5 = 294,
        F6 = 295,
        F7 = 296,
        F8 = 297,
        F9 = 298,
        F10 = 299,
        F11 = 300,
        F12 = 301,
        F13 = 302,
        F14 = 303,
        F15 = 304,
        F16 = 305,
        F17 = 306,
        F18 = 307,
        F19 = 308,
        F20 = 309,
        F21 = 310,
        F22 = 311,
        F23 = 312,
        F24 = 313,
        F25 = 314,
        Numpad0 = 320,
        Numpad1 = 321,
        Numpad2 = 322,
        Numpad3 = 323,
        Numpad4 = 324,
        Numpad5 = 325,
        Numpad6 = 326,
        Numpad7 = 327,
        Numpad8 = 328,
        Numpad9 = 329,
        NumpadDecimal = 330,
        NumpadDivide = 331,
        NumpadMultiply = 332,
        NumpadSubtract = 333,
        NumpadAdd = 334,
        NumpadEnter = 335,
        NumpadEqual = 336,
        ShiftLeft = 340,
        ControlLeft = 341,
        AltLeft = 342,
        SuperLeft = 343,
        ShiftRight = 344,
        ControlRight = 345,
        AltRight = 346,
        SuperRight = 347,
        Menu = 348,
        First = Space,
        Last = Menu
    };

    enum class EInputAction : int32_t
    {
        Release = 0,
        Press = 1,
        Repeat = 2
    };

    enum class EInputMod : int32_t
    {
        None = 0,
        Shift = 1,
        Control = 2,
        Alt = 4,
        Super = 8
    };

    class Keyboard
    {
    public:
        explicit Keyboard(Window* window);
        void doChar(uint32_t codepoint);
        void doKey(int32_t key, int32_t action, int32_t mods);

        MulticastDelegate<char> onChar;
        MulticastDelegate<EKey, EInputAction, EInputMod> onKey;

    private:
        Window* window_;
    };
}
#endif //AMX_KEYBOARD_HPP
