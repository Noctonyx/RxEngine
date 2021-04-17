//
// Created by shane on 21/02/2021.
//

#ifndef RX_IINPUTHANDLER_H
#define RX_IINPUTHANDLER_H

#include "Input/Keyboard.hpp"
namespace RxEngine
{
    class IInputHandler
    {
    public:
        virtual bool OnMouseButton(int32_t button, bool pressed, RxEngine::EInputMod mods) { return false; }
        virtual bool OnMousePos(float x, float y, RxEngine::EInputMod mods) { return false; }
        virtual bool OnMouseScroll(float yscroll, RxEngine::EInputMod mods) { return false; }
        virtual bool OnKey(
            RxEngine::EKey key,
            RxEngine::EInputAction action,
            RxEngine::EInputMod mods) { return false; }
        virtual bool OnChar(char c) { return false; }
    };
}

#endif //RX_IINPUTHANDLER_H
