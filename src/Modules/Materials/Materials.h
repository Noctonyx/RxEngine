#pragma once

#include "Modules/Module.h"
#include "RxCore.h"
#include "RXAssets.h"

namespace RxEngine
{
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

    class MaterialsModule : public Module
    {
    public:
        MaterialsModule(ecs::World* world, EngineMain* engine)
            : Module(world, engine) {}

        void startup() override;
        void shutdown() override;

        vk::Pipeline createMaterialPipeline(const MaterialPipelineDetails * mpd,
                                            const FragmentShader * frag,
                                            const VertexShader * vert,
                                            vk::PipelineLayout layout,
                                            vk::RenderPass rp,
                                            uint32_t subpass);
    };
}
