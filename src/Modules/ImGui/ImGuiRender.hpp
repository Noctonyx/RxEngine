//
// Created by shane on 3/04/2020.
//

#ifndef AMX_IMGUIRENDER_HPP
#define AMX_IMGUIRENDER_HPP

#include <vector>
#include <memory>
#include "Modules/Module.h"
#include <imgui.h>
//#include "Delegates.hpp"
#include "Input/Keyboard.hpp"
#include "Rendering/Renderer.hpp"

namespace RxEngine
{
    class Scene;
    class Keyboard;
    class Mouse;

    class IMGuiRender : public Module
    {
    public:
        IMGuiRender(ecs::World* world, EngineMain* engine);
        ~IMGuiRender();

        void startup() override;
        void shutdown() override;

        void SetupInputs(ImGuiIO & io);
        void CreateFontImage(ImGuiIO & io);
        //void rendererInit(Renderer * renderer) override;
        void update(float deltaTime);
        void updateGui();

        void createRenderCommands();

        //void Shutdown() override;

    protected:
        [[nodiscard]] std::tuple<std::shared_ptr<RxCore::VertexBuffer>, std::shared_ptr<
                                     RxCore::IndexBuffer>> CreateBuffers() const;

        void createMaterial(vk::RenderPass renderPass);

    private:
        std::vector<DelegateHandle> delegates_{};
        std::shared_ptr<RxCore::Image> fontImage_;
        //std::shared_ptr<Mouse> mouse_;
        Window * window_;
        //std::shared_ptr<Keyboard> keyboard_;
        //std::shared_ptr<Material> _material;

        vk::DescriptorSetLayout dsl0;
        vk::PipelineLayout pipelineLayout;
        std::shared_ptr<RxCore::DescriptorSet> set0;
        //std::shared_ptr<RxCore::Pipeline> pipeline;
        ecs::entity_t pipelineEntity{};

        bool show_demo_window = true;
        bool show_another_window = true;

#if 0
        std::shared_ptr<RXCore::PipelineLayout> CreatePipelineLayout();

        std::shared_ptr<RXCore::Pipeline> createPipeline(
            std::shared_ptr<RXCore::PipelineLayout> layout,
            const std::shared_ptr<RXCore::RenderPass> & renderPass);
#endif
    };
}
#endif // AMX_IMGUIRENDER_HPP
