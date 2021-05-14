//
// Created by shane on 21/02/2021.
//

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Debugger/Debugger.h>
#include "UiContext.h"
#include "Window.hpp"
#include "optick/optick.h"

namespace RxEngine
{
    UiContext::UiContext(ecs::World * world, EngineMain * engine)
        : Module(world, engine)
    {
        auto wd = world_->getSingleton<WindowDetails>();
        context_ = Rml::CreateContext(
            "MainUI", Rml::Vector2i(static_cast<int>(wd->width), static_cast<int>(wd->height)));
        Rml::Debugger::Initialise(context_);
    }

    Rml::ElementDocument * UiContext::addDocument(const std::string & document)
    {
        auto doc = context_->LoadDocument(document);
        documents_.push_back(doc);
        doc->Show();

        return doc;
    }

    UiContext::~UiContext()
    {
        for (auto & d: documents_) {
            d->Close();
        }
        documents_.clear();
    }

    void UiContext::startup()
    {
        world_->createSystem("UiContext:Render")
              .inGroup("Pipeline:PreRender")
              .execute([&](ecs::World * world)
              {
                  OPTICK_EVENT()
                  context_->Render();
              });

        world_->createSystem("UiContext:WindowResize")
              .withStream<WindowResize>()
              .inGroup("Pipeline:Update")
              .execute<WindowResize>([&](ecs::World * world, const WindowResize * resize)
              {
                  OPTICK_EVENT()

                  context_->SetDimensions(
                      Rml::Vector2i{
                          static_cast<int>(resize->width),
                          static_cast<int>(resize->height)
                      });

                  context_->Update();
                  return false;
              });

        world_->createSystem("UiContext:MousePos")
              .withStream<MousePosition>()
              .inGroup("Pipeline:Early")
              .execute<MousePosition>([&](ecs::World * world, const MousePosition * pos)
              {
                  OPTICK_EVENT()
                  return !context_->ProcessMouseMove(static_cast<int>(pos->x),
                                                     static_cast<int>(pos->y),
                                                     convertModState(pos->mods));
              });

        world_->createSystem("UiContext:MouseButton")
              .withStream<MouseButton>()
              .inGroup("Pipeline:Early")
              .execute<MouseButton>([&](ecs::World * world, const MouseButton * button)
              {
                  OPTICK_EVENT()
                  if (button->pressed) {
                      return !context_->ProcessMouseButtonDown(
                          button->button, convertModState(button->mods));
                  } else {
                      return !context_->ProcessMouseButtonUp(
                          button->button, convertModState(button->mods));
                  }
              });

        world_->createSystem("UiContext:MouseScroll")
              .withStream<MouseScroll>()
              .inGroup("Pipeline:Early")
              .execute<MouseScroll>([&](ecs::World * world, const MouseScroll * s)
              {
                  OPTICK_EVENT()
                  return !context_->ProcessMouseWheel(- s->y_offset, convertModState(s->mods));
              });

        world_->createSystem("UiContext:Key")
              .withStream<KeyboardKey>()
              .inGroup("Pipeline:Early")
              .execute<KeyboardKey>([&](ecs::World * world, const KeyboardKey * key)
              {
                  OPTICK_EVENT()
                  if (key->key == EKey::F8 && key->action == EInputAction::Press) {
                      Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
                  }
                  if (key->key == EKey::F9 && key->action == EInputAction::Press) {
                      for (auto & d: documents_) {
                          d->ReloadStyleSheet();
                      }
                  }
                  if (key->action == EInputAction::Press) {
                      return !context_->ProcessKeyDown(convertKey(key->key),
                                                       convertModState(key->mods));
                  }
                  if (key->action == EInputAction::Release) {
                      return !context_->ProcessKeyUp(convertKey(key->key),
                                                     convertModState(key->mods));
                  }

                  return false;
              });

        world_->createSystem("UiContext:Char")
              .withStream<KeyboardChar>()
              .inGroup("Pipeline:Early")
              .execute<KeyboardChar>([&](ecs::World * world, const KeyboardChar * c)
              {
                  OPTICK_EVENT()
                  return !context_->ProcessTextInput(c->c);
              });
    }

    void UiContext::shutdown()
    {
        world_->deleteSystem(world_->lookup("UiContext:Render").id);
        world_->deleteSystem(world_->lookup("UiContext:WindowResize").id);
        world_->deleteSystem(world_->lookup("UiContext:MousePos").id);
        world_->deleteSystem(world_->lookup("UiContext:MouseButton").id);
        world_->deleteSystem(world_->lookup("UiContext:MouseScroll").id);
        world_->deleteSystem(world_->lookup("UiContext:Key").id);
        world_->deleteSystem(world_->lookup("UiContext:Char").id);
    }

    int UiContext::convertModState(RxEngine::EInputMod mods)
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

    Rml::Input::KeyIdentifier UiContext::convertKey(EKey key)
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
}
