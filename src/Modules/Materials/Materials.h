#pragma once

#include "Modules/Module.h"
#include "RXCore.h"
#include "RXAssets.h"

namespace RxEngine
{
    struct RenderPasses;

    struct ShadowPipeline
    {
        std::shared_ptr<RxCore::Pipeline> pipeline;
        vk::RenderPass renderPass;
        uint32_t subPass;
    };

    struct OpaquePipeline
    {
        std::shared_ptr<RxCore::Pipeline> pipeline;
        vk::RenderPass renderPass;
        uint32_t subPass;
    };

    struct TransparentPipeline
    {
        std::shared_ptr<RxCore::Pipeline> pipeline;
        vk::RenderPass renderPass;
        uint32_t subPass;
    };

    struct UiPipeline
    {
        std::shared_ptr<RxCore::Pipeline> pipeline;
        vk::RenderPass renderPass;
        uint32_t subPass;
    };

    struct VertexShader
    {
        std::shared_ptr<RxCore::Shader> shader;
        std::string shaderAssetName{};
    };

    struct FragmentShader
    {
        std::shared_ptr<RxCore::Shader> shader;
        std::string shaderAssetName{};
    };

    struct PipelineLayout
    {
        vk::PipelineLayout layout;
        std::vector<vk::DescriptorSetLayout> dsls;
    };

    struct UsesFragmentShader : ecs::Relation {};
    struct UsesVertexShader : ecs::Relation {};
    struct UsesLayout : ecs::Relation {};

    struct HasOpaquePipeline : ecs::Relation { };
    struct HasShadowPipeline : ecs::Relation { };
    struct HasTransparentPipeline : ecs::Relation { };
    struct HasUiPipeline : ecs::Relation { };
    struct HasPipeline {};

    struct MaterialPipelineDetails
    {
        float lineWidth;
        RxAssets::MaterialPipelineFillMode fillMode;
        bool depthClamp;
        RxAssets::MaterialPipelineCullMode cullMode;
        RxAssets::MaterialPipelineFrontFace frontFace;

        bool depthTestEnable;
        bool depthWriteEnable;
        RxAssets::MaterialPipelineDepthCompareOp depthCompareOp;

        bool stencilTest;
        float minDepth;
        float maxDepth;

        std::vector<RxAssets::MaterialPipelineAttachmentBlend> blends;
        std::vector<RxAssets::MaterialPipelineInput> inputs;

        RxAssets::PipelineRenderStage stage;
    };

    struct MaterialImage
    {
        std::shared_ptr<RxCore::Image> image;
        std::shared_ptr<RxCore::ImageView> imageView;
    };

    enum class MaterialAlphaMode : uint8_t
    {
        Opaque,
        Transparent
    };

    struct Material
    {
        std::array<ecs::entity_t, 4> materialTextures{ 0, 0, 0, 0 };
        float roughness;
        float metallic;
        MaterialAlphaMode alpha;
    };


    class MaterialsModule : public Module
    {
    public:
        MaterialsModule(ecs::World* world, EngineMain* engine)
            : Module(world, engine) {}

        void registerModule() override;
        void deregisterModule() override;
        void startup() override;
        void shutdown() override;

        void processStartupData(sol::state * lua, RxCore::Device* device) override;

        static vk::Pipeline createMaterialPipeline(const MaterialPipelineDetails * mpd,
                                                   const FragmentShader * frag,
                                                   const VertexShader * vert,
                                                   vk::PipelineLayout layout,
                                                   vk::RenderPass rp,
                                                   uint32_t subpass);

        static void createPipelines(ecs::EntityHandle e,
            const MaterialPipelineDetails* mpd,
            const FragmentShader* frag,
            const VertexShader* vert,
            const PipelineLayout* pll,
            const RenderPasses* rp);

    private:
        static void materialGui(ecs::EntityHandle e);
    };
}
