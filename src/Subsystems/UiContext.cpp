//
// Created by shane on 21/02/2021.
//

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Debugger/Debugger.h>
#include "UiContext.h"
#include "Window.hpp"
#include "Scene.h"

namespace RxEngine
{
    void UiContext::rendererInit(Renderer * renderer)
    {

    }

    void UiContext::preRender(uint32_t width, uint32_t height)
    {
        OPTICK_EVENT()
        context_->Render();
    }

    void UiContext::UpdateGui()
    {
        OPTICK_EVENT()

        context_->SetDimensions(
            Rml::Vector2i{
                static_cast<int>(scene_->getWidth()),
                static_cast<int>(scene_->getHeight())
            });

        context_->Update();
    }

    UiContext::UiContext(const std::string & name, uint32_t width, uint32_t height)
        : context_(Rml::CreateContext(name, Rml::Vector2i(static_cast<int>(width), static_cast<int>(height))))
    {
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

    bool UiContext::OnMouseButton(int32_t button, bool pressed, RxEngine::EInputMod mods)
    {
        if (pressed) {
            return !context_->ProcessMouseButtonDown(button, convertModState(mods));
        } else {
            return !context_->ProcessMouseButtonUp(button, convertModState(mods));
        }
    }

    bool UiContext::OnMousePos(float x, float y, RxEngine::EInputMod mods)
    {
        //spdlog::info("UI Mouse move");
        return !context_->ProcessMouseMove(static_cast<int>(x), static_cast<int>(y), convertModState(mods));
    }

    bool UiContext::OnMouseScroll(float yscroll, RxEngine::EInputMod mods)
    {
        return !context_->ProcessMouseWheel(-yscroll, convertModState(mods));
    }

    bool UiContext::OnKey(RxEngine::EKey key, RxEngine::EInputAction action, RxEngine::EInputMod mods)
    {
        if (key == EKey::F8 && action == EInputAction::Press) {
            Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
        }
        if (key == EKey::F9 && action == EInputAction::Press) {
            for (auto & d: documents_) {
                d->ReloadStyleSheet();
            }
        }
        if (action == EInputAction::Press) {
            return !context_->ProcessKeyDown(convertKey(key), convertModState(mods));
        }
        if (action == EInputAction::Release) {
            return !context_->ProcessKeyUp(convertKey(key), convertModState(mods));
        }

        return false;
    }

    bool UiContext::OnChar(char c)
    {
        return !context_->ProcessTextInput(c);
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

    std::vector<RenderEntity> UiContext::getRenderEntities()
    {
        return {};
    }
}