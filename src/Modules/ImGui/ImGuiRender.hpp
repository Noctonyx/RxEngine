//
// Created by shane on 3/04/2020.
//

#pragma once

#include <vector>
#include <memory>
#include "Modules/Module.h"
#include <imgui.h>
#include "Window.hpp"
#include "RXCore.h"
#include "RxECS.h"

namespace RxEngine
{
    class IMGuiRender : public Module
    {
    public:
        IMGuiRender(ecs::World * world, EngineMain * engine);
        ~IMGuiRender() override;

        void startup() override;
        void shutdown() override;

        void setupInputs(ImGuiIO & io);
        void createFontImage(ImGuiIO & io);
        void createDescriptorSet();
        void update(float deltaTime);
        void updateGui();

        void createRenderCommands();

        bool isEnabled() const  
        {
            return enabled;
        }

    private:
        //std::vector<DelegateHandle> delegates_{};
        std::shared_ptr<RxCore::Image> fontImage_{};

        RxCore::Window * window_{};

        std::shared_ptr<RxCore::DescriptorSet> set0_;
        ecs::EntityHandle pipeline_;

        bool enabled = false;
        //bool showDemoWindow_ = true;
        //bool showAnotherWindow_ = false;
    };
}
