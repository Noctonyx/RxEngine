//
// Created by shane on 21/02/2021.
//

#ifndef RX_UICONTEXT_H
#define RX_UICONTEXT_H

#include <Rendering/Renderer.hpp>
#include <Subsystem.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Input.h>
#include "Subsystems/IInputHandler.h"

namespace RxEngine
{
    class UiContext : public Subsystem, public IRenderable, public IInputHandler
    {
    public:
        UiContext(const std::string & name, uint32_t width, uint32_t height);
        virtual ~UiContext();

        void UpdateGui() override;
        void rendererInit(Renderer * renderer) override;
        void preRender(uint32_t width, uint32_t height) override;

        Rml::ElementDocument * addDocument(const std::string & document);

        bool OnMouseButton(int32_t button, bool pressed, RxEngine::EInputMod mods) override;
        bool OnMousePos(float x, float y, RxEngine::EInputMod mods) override;
        bool OnMouseScroll(float yscroll, RxEngine::EInputMod mods) override;
        bool OnKey(RxEngine::EKey key, RxEngine::EInputAction action, RxEngine::EInputMod mods) override;
        bool OnChar(char c) override;

        static int convertModState(RxEngine::EInputMod mods);
        static Rml::Input::KeyIdentifier convertKey(EKey key);

        std::vector<RenderEntity> getRenderEntities() override;
    private:
        Rml::Context * context_;
        std::vector<Rml::ElementDocument *> documents_{};
    };
}
#endif //RX_UICONTEXT_H
