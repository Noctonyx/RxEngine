#pragma once

#include <memory>


namespace RxEngine
{
#if 0
    class TestMesh : public Subsystem, public RXCore::IRenderable
    {
    public:

        explicit TestMesh()
            : modelMat()
            , position(glm::vec3(0.f))
            , uboScene()
            , stats_(nullptr) {}

        void RendererInit(const RXCore::Renderer * renderer) override;

        void Startup(const std::vector<Subsystem *> & subsystems) override;

        void Update(float delta) override;

        void UpdateGui() override;

        void BindMaterial(std::shared_ptr<RXCore::SecondaryCommandBuffer> buf3,
                          RXCore::RenderSequence sequence,
                          std::shared_ptr<Material> mb) const;

        void DrawMesh(std::shared_ptr<RXCore::SecondaryCommandBuffer> buf3,
                      std::shared_ptr<Mesh> mesh) const;

        //void SetCameraData(std::shared_ptr<RXCore::SecondaryCommandBuffer> buf3) const;

        void DrawAllMeshes(RXCore::RenderSequence sequence,
                           std::shared_ptr<RXCore::SecondaryCommandBuffer> buf3) const;


        void PreRender(uint32_t width, uint32_t height) override
        {
            viewPortHeight_ = height;
            viewPortWidth_ = width;
        };

        [[nodiscard]] RXCore::RenderResponse Render(
            RXCore::RenderSequence sequence,
            const RXCore::RenderStage & stage) const override;

        void Shutdown() override;

        void CreateMeshBuffers();

        void BuildPipeline(std::shared_ptr<RXCore::RenderPass> & renderPass);

    private:
        std::shared_ptr<RXEngine::Material> triangleMaterial;
        //std::shared_ptr<RXCore::Pipeline> trianglePipeline_;
        std::shared_ptr<Mesh> mesh_;
        std::vector<std::shared_ptr<RenderMesh>> rm_;

        std::shared_ptr<RXCore::Buffer> uboCamera_;
        std::shared_ptr<RXCore::Buffer> uboModelMatrix_;

        uint32_t viewPortWidth_, viewPortHeight_;

        float xRot_ = 0.f;
        float yRot_ = 0.f;
        float zRot_ = 0.f;

        glm::mat4 modelMat;

        std::shared_ptr<RXCore::Camera> sceneCamera_;

        glm::vec3 position;

        struct UboScene
        {
            glm::mat4 projection;
            glm::mat4 view;
            glm::vec3 viewPos;
        } uboScene;

        Stats * stats_;
    };
#endif
} // namespace RXCore
