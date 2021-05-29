#include "RmlUI.h"

#include "UiContext.h"
#include "Window.hpp"
#include "optick/optick.h"
#include "RmlUi/Core/Core.h"
#include "RmlUi/Lua/Lua.h"

namespace RxEngine
{
    void RmlUiModule::registerModule() { }

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
              .inGroup("Pipeline:Update")
              .withStream<WindowResize>()
              .execute<WindowResize>(
                  [&](ecs::World *, const WindowResize * resize)
                  {
                      rmlRender->setDirty();
                      return false;
                  }
              );

        world_->createSystem("Rml:Render")
              .inGroup("Pipeline:PreRender")
              .withRead<UiContextProcessed>()
              .execute([this](ecs::World *)
              {
                  OPTICK_EVENT("Rml:Render")
                  rmlRender->renderUi(world_);
              });
    }

    void RmlUiModule::shutdown()
    {
        world_->deleteSystem(world_->lookup("RmlUI:Resize"));

        Rml::Shutdown();

        rmlRender.reset();
        rmlSystem.reset();
        rmlFile.reset();
    }

    void RmlUiModule::deregisterModule() { }
}
