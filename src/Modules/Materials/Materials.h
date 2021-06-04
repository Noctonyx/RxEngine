#pragma once

#include "Modules/Module.h"
#include "RxCore.h"
#include "RXAssets.h"

namespace RxEngine
{
    struct DescriptorSet;
    struct RenderPasses;

    struct GraphicsPipeline
    {
        RxApi::PipelinePtr pipeline;
        RxApi::RenderPass renderPass;
        uint32_t subPass;
    };
#if 0
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
#endif
    struct VertexShader
    {
        RxApi::ShaderPtr shader;
        std::string shaderAssetName{};
    };

    struct FragmentShader
    {
        RxApi::ShaderPtr shader;
        std::string shaderAssetName{};
    };

    struct PipelineLayout
    {
        RxApi::PipelineLayout layout;
        std::vector<RxApi::DescriptorSetLayout> dsls;
    };

    struct UsesFragmentShader : ecs::Relation {};

    struct UsesVertexShader : ecs::Relation {};

    struct UsesLayout : ecs::Relation {};

    struct HasOpaquePipeline : ecs::Relation { };

    struct HasShadowPipeline : ecs::Relation { };

    struct HasTransparentPipeline : ecs::Relation { };

    struct HasUiPipeline : ecs::Relation { };

    //struct HasPipeline {};

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
        RxApi::ImagePtr image;
        RxApi::ImageViewPtr imageView;
    };

    enum class MaterialAlphaMode : uint8_t
    {
        Opaque,
        Transparent
    };

    struct Material
    {
        std::array<ecs::entity_t, 4> materialTextures{0, 0, 0, 0};
        //ecs::entity_t materialTexture;
        float roughness;
        float metallic;
        MaterialAlphaMode alpha;
        uint32_t sequence{};
    };

    struct MaterialDescriptor { };

    struct MaterialShaderEntry
    {
        uint32_t colorTextureIndex;
        float roughness;
    };

    class MaterialsModule : public Module
    {
    public:
        MaterialsModule(ecs::World * world, EngineMain * engine, const ecs::entity_t moduleId)
            : Module(world, engine, moduleId) {}

        void startup() override;
        void shutdown() override;

        void loadData(sol::table table) override;
        //void processStartupData(sol::state * lua, RxCore::Device * device) override;

        static RxApi::PipelinePtr createMaterialPipeline(const MaterialPipelineDetails * mpd,
                                                   const FragmentShader * frag,
                                                   const VertexShader * vert,
                                                   RxApi::PipelineLayout layout,
                                                   RxApi::RenderPass rp,
                                                   uint32_t subpass);

        static void createPipelines(ecs::EntityHandle e,
                                    const MaterialPipelineDetails * mpd,
                                    const FragmentShader * frag,
                                    const VertexShader * vert,
                                    const PipelineLayout * pll,
                                    const RenderPasses * rp);

        void createShaderMaterialData(ecs::EntityHandle e, DescriptorSet * ds);
    private:
        ecs::queryid_t materialQuery;
        //static void materialGui(ecs::EntityHandle e);
    };
}
