#include "RmlUI.h"

#include "Window.hpp"
#include "RmlUi/Core/Core.h"
#include "RmlUi/Lua/Lua.h"

namespace RxEngine
{
    void RmlUiModule::registerModule()
    {
     
    }

    void RmlUiModule::startup()
    {
        rmlSystem = std::make_unique<RmlSystemInterface>(world_);
        rmlFile = std::make_unique<RmlFileInterface>();
        rmlRender = std::make_unique<RmlRenderInterface>();

        Rml::SetSystemInterface(rmlSystem.get());
        Rml::SetFileInterface(rmlFile.get());
        Rml::SetRenderInterface(rmlRender.get());
        Rml::Initialise();

        Rml::LoadFontFace("/ui/fonts/TitilliumWeb-Regular.ttf");
        Rml::LoadFontFace("/ui/LatoLatin-Regular.ttf");
        Rml::LoadFontFace("/ui/fonts/Roboto-Regular.ttf");
        Rml::LoadFontFace("/ui/fonts/Roboto-Bold.ttf");
        Rml::Lua::Initialise();

        world_->createSystem("RmlUI:Resize")
            .withStream<WindowResize>()
            .execute<WindowResize>(
                [&](ecs::World *, const WindowResize* resize)
                {
                    rmlRender->setDirty();
                    return false;
                }
        );
    }

    void RmlUiModule::shutdown()
    {
        world_->lookup("RmlUI:Resize").destroy();

        Rml::Shutdown();

        rmlRender.reset();
        rmlSystem.reset();
        rmlFile.reset();
    }

    void RmlUiModule::deregisterModule()
    {
    
    }
}
