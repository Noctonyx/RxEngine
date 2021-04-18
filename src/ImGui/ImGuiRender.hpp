//
// Created by shane on 3/04/2020.
//

#ifndef AMX_IMGUIRENDER_HPP
#define AMX_IMGUIRENDER_HPP

#include <vector>
#include <memory>
#include "Subsystem.h"
#include <imgui.h>
#include "Delegates.hpp"
#include "Rendering/Renderer.hpp"

namespace RxEngine
{
    class Scene;
    class Keyboard;
    class Mouse;

    class IMGuiRender : public Subsystem, public IRenderable, public IInputHandler
    {
        //friend class Engine;//

    public:
        IMGuiRender(std::shared_ptr<Mouse> mouse, std::shared_ptr<Keyboard> keyboard);

        ~IMGuiRender();

        int16_t GetCallPriority(Sequences sequence) override
        {
            switch (sequence) {
                case Sequences::RendererInit:
                    return -1;
                case Sequences::Update:
                    return -1;
                case Sequences::UpdateGui:
                    return -1;
//            case Sequences::Shutdown:
                    //              return 1000;
                default:
                    return Subsystem::GetCallPriority(sequence);
            }
        }

        bool OnMouseButton(int32_t button, bool pressed, EInputMod mods) override;
        bool OnMousePos(float x, float y, EInputMod mods) override;
        bool OnMouseScroll(float yscroll, EInputMod mods) override;
        bool OnKey(EKey key, EInputAction action, EInputMod mods) override;
        bool OnChar(char c) override;
        void SetupInputs(ImGuiIO & io);
        void CreateFontImage(ImGuiIO & io);
        void rendererInit(Renderer * renderer) override;
        void Update(float deltaTime) override;
        void UpdateGui() override;
        void WindowResize(int width, int height);

        bool hasRenderUi() const override { return true; }
        [[nodiscard]] RenderResponse renderUi(
            const RenderStage & stage,
            const uint32_t width,
            const uint32_t height) override;

        //void Shutdown() override;

        void addedToScene(
            Scene * scene,
            std::vector<std::shared_ptr<Subsystem>> & subsystems) override;
        void removedFromScene() override;
    protected:
        [[nodiscard]] std::tuple<std::shared_ptr<RxCore::VertexBuffer>, std::shared_ptr<
            RxCore::IndexBuffer>> CreateBuffers() const;

        void createMaterial(vk::RenderPass renderPass);

    public:
        std::vector<RenderEntity> getRenderEntities() override;
    private:
        std::vector<DelegateHandle> delegates_;
        std::shared_ptr<RxCore::Image> fontImage_;
        std::shared_ptr<Mouse> mouse_;
        //std::shared_ptr<Keyboard> keyboard_;
        //std::shared_ptr<Material> _material;

        vk::DescriptorSetLayout dsl0;
        vk::PipelineLayout pipelineLayout;
        std::shared_ptr<RxCore::DescriptorSet> set0;
        vk::Pipeline pipeline;

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
