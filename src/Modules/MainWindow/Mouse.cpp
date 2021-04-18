#include "Mouse.h"
#include "Input/Mouse.hpp"

#include "EngineMain.hpp"
#include "MainWindow.h"
#include "Window.hpp"

namespace RxEngine
{
    void MouseModule::startup()
    {
        auto w = engine_->getWindow();

        w->mouse->onMousePos.AddRaw(this, &MouseModule::onMove);
        w->mouse->onMouseButton.AddRaw(this, &MouseModule::onButton);
        w->mouse->onScroll.AddRaw(this, &MouseModule::onScroll);
    }

    void MouseModule::shutdown()
    {
        auto w = engine_->getWindow();

        w->mouse->onMousePos.RemoveObject(this);
        w->mouse->onMouseButton.RemoveObject(this);
        w->mouse->onScroll.RemoveObject(this);
    }
}
