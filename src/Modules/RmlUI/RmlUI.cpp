#include "RmlUI.h"

#include "Window.hpp"
#include "optick/optick.h"
#include "RmlUi/Core/Context.h"
#include "RmlUi/Core/Core.h"
#include "RmlUi/Core/ElementDocument.h"
#include "RmlUi/Core/Input.h"
#include "RmlUi/Debugger/Debugger.h"
#include "RmlUi/Lua/Lua.h"

namespace RxEngine
{
    int convertModState(RxEngine::EInputMod mods)
    {
        int kms = 0;
        if (static_cast<uint32_t>(mods) & static_cast<uint32_t>(EInputMod::Shift)) {
            kms |= Rml::Input::KeyModifier::KM_SHIFT;
        }
        if (static_cast<uint32_t>(mods) & static_cast<uint32_t>(EInputMod::Control)) {
            kms |= Rml::Input::KeyModifier::KM_CTRL;
        }
        if (static_cast<uint32_t>(mods) & static_cast<uint32_t>(EInputMod::Alt)) {
            kms |= Rml::Input::KeyModifier::KM_ALT;
        }
        return kms;
    }

    Rml::Input::KeyIdentifier convertKey(EKey key)
    {
        switch (key) {
        case EKey::Space:
            return Rml::Input::KeyIdentifier::KI_SPACE;
        case EKey::Apostrophe:
            return Rml::Input::KeyIdentifier::KI_OEM_3;
        case EKey::Comma:
            return Rml::Input::KeyIdentifier::KI_OEM_COMMA;
        case EKey::F1:
            return Rml::Input::KeyIdentifier::KI_F1;
        case EKey::F2:
            return Rml::Input::KeyIdentifier::KI_F2;
        case EKey::F3:
            return Rml::Input::KeyIdentifier::KI_F3;
        case EKey::F4:
            return Rml::Input::KeyIdentifier::KI_F4;
        case EKey::F5:
            return Rml::Input::KeyIdentifier::KI_F5;
        case EKey::F6:
            return Rml::Input::KeyIdentifier::KI_F6;
        case EKey::F7:
            return Rml::Input::KeyIdentifier::KI_F7;
        case EKey::F8:
            return Rml::Input::KeyIdentifier::KI_F8;
        case EKey::F9:
            return Rml::Input::KeyIdentifier::KI_F9;
        case EKey::F10:
            return Rml::Input::KeyIdentifier::KI_F10;

        default:
            return Rml::Input::KeyIdentifier::KI_UNKNOWN;
        }
    }

    Rml::ElementDocument * UiContext::loadDocument(const std::string & document)
    {
        auto doc = context->LoadDocument(document);
        documents.push_back({document, doc});
        doc->Show();

        return doc;
    }

    void UiContext::closeDocument(const std::string & document) { }

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

        world_->createSystem("RmlUI:NewContext")
              .inGroup("Pipeline:PreFrame")
              .withQuery<UiContext>()
              .without<UiContextCreated>()
              .with<ecs::Name>()
              .withSingleton<WindowDetails>()
              .each<UiContext, ecs::Name, WindowDetails>(
                  [](ecs::EntityHandle e,
                     UiContext * ctx,
                     const ecs::Name * name,
                     const WindowDetails * wd)
                  {
                      ctx->context = Rml::CreateContext(
                          name->name.c_str(),
                          Rml::Vector2i(static_cast<int>(wd->width), static_cast<int>(wd->height)));
                      e.addDeferred<UiContextCreated>();
                      if (ctx->debugger) {
                          Rml::Debugger::Initialise(ctx->context);
                      }
                  });


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

        world_->createSystem("RmlUI:Mouse")
              .withQuery<UiContext, UiContextCreated, UiContextInteractive>()
              .inGroup("Pipeline:Early")
              .withWrite<MousePosition>()
              .withWrite<MouseButton>()
              .withWrite<MouseScroll>()
              .each<UiContext>([&](ecs::EntityHandle e, const UiContext * ctx)
              {
                  OPTICK_EVENT()
                  e.world->getStream<MousePosition>()->each<MousePosition>(
                      [&](ecs::World *, const MousePosition * mp)
                      {
                          return !ctx->context->ProcessMouseMove(static_cast<int>(mp->x),
                                                                 static_cast<int>(mp->y),
                                                                 convertModState(mp->mods));
                      });

                  e.world->getStream<MouseButton>()->each<MouseButton>(
                      [&](ecs::World *, const MouseButton * button)
                      {
                          if (button->pressed) {
                              return !ctx->context->ProcessMouseButtonDown(
                                  button->button, convertModState(button->mods));
                          } else {
                              return !ctx->context->ProcessMouseButtonUp(
                                  button->button, convertModState(button->mods));
                          }
                      });

                  e.world->getStream<MouseScroll>()->each<MouseScroll>(
                      [&](ecs::World *, const MouseScroll * s)
                      {
                          return !ctx->context->ProcessMouseWheel(
                              -s->y_offset, convertModState(s->mods));
                      });
              });

        world_->createSystem("RmlUI:Key")
              .withQuery<UiContext, UiContextCreated, UiContextInteractive>()
              .inGroup("Pipeline:Early")
              .withWrite<KeyboardKey>()
              .withWrite<KeyboardChar>()
              .each<UiContext>([&](ecs::EntityHandle e, UiContext * ctx)
              {
                  OPTICK_EVENT()
                  e.world->getStream<KeyboardKey>()->each<KeyboardKey>(
                      [&](ecs::World *, const KeyboardKey * key)
                      {
                          if (key->key == EKey::F8 && key->action == EInputAction::Press) {
                              Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
                          }
                          if (key->key == EKey::F9 && key->action == EInputAction::Press) {
                              for (auto & d: ctx->documents) {
                                  d.second->ReloadStyleSheet();
                              }
                          }
                          if (key->key == EKey::F10 && key->action == EInputAction::Press) {
                              for (auto & d: ctx->documents) {

                                  d.second->Close();
                                  d.second = ctx->context->LoadDocument(d.first);
                                  d.second->Show();
                              }
                          }
                          if (key->action == EInputAction::Press) {
                              return !ctx->context->ProcessKeyDown(convertKey(key->key),
                                  convertModState(key->mods));
                          }
                          if (key->action == EInputAction::Release) {
                              return !ctx->context->ProcessKeyUp(convertKey(key->key),
                                                                 convertModState(key->mods));
                          }

                          return false;
                      });

                  e.world->getStream<KeyboardChar>()->each<KeyboardChar>(
                      [&](ecs::World *, const KeyboardChar * c)
                      {
                          return ctx->context->ProcessTextInput(c->c);
                      });
              });


        world_->createSystem("Rml:RenderContext")
              .inGroup("Pipeline:PreRender")
              .withQuery<UiContext, UiContextCreated>()
              .withWrite<UiContextProcessed>()
              .each<UiContext>([&](ecs::EntityHandle e, const UiContext * ctx)
              {
                  OPTICK_EVENT()
                  ctx->context->Update();
                  ctx->context->Render();
              });

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
        world_->deleteSystem(world_->lookup("RmlUI:Render"));
        world_->deleteSystem(world_->lookup("RmlUI:RenderContext"));
        world_->deleteSystem(world_->lookup("RmlUI:Key"));
        world_->deleteSystem(world_->lookup("RmlUI:Mouse"));
        world_->deleteSystem(world_->lookup("RmlUI:NewContext"));

        auto q = world_->createQuery().with<UiContext, UiContextCreated, ecs::Name>();

        world_->getResults(q.id).each<ecs::Name>([](ecs::EntityHandle e, ecs::Name * name)
        {
            Rml::RemoveContext(name->name);
        });

        Rml::Shutdown();

        rmlRender.reset();
        rmlSystem.reset();
        rmlFile.reset();
    }

    void RmlUiModule::deregisterModule() { }
}
