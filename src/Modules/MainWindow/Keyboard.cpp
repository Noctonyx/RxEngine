#include "Keyboard.h"
#include "Input/Keyboard.hpp"

#include "MainWindow.h"
#include "Window.hpp"

namespace RxEngine
{
    void KeyboardModule::startup()
    {
        auto wd = world_->getSingleton<MainWindow::WindowDetails>();
        assert(wd);
        wd->window->keyboard->onKey.AddRaw(this, &KeyboardModule::onKey);
        wd->window->keyboard->onChar.AddRaw(this, &KeyboardModule::onChar);
    }

    void KeyboardModule::shutdown()
    {
        auto wd = world_->getSingleton<MainWindow::WindowDetails>();
        assert(wd);
        wd->window->keyboard->onChar.RemoveObject(this);
        wd->window->keyboard->onKey.RemoveObject(this);
    }

    void KeyboardModule::onKey(EKey key, EInputAction action, EInputMod mods)
    {
        auto s = world_->getStream<KeyboardKey>();

        s->add<KeyboardKey>({ static_cast<EKey>(key), static_cast<EInputAction> (action), static_cast<EInputMod>(mods) });
    }

    void KeyboardModule::onChar(char c)
    {
        auto s = world_->getStream<KeyboardChar>();
        s->add<KeyboardChar>({ static_cast<char>(c)});
    }
}
