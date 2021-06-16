#pragma once

#include "Modules/Module.h"
#include "RXCore.h"
#include "RXAssets.h"

namespace RxEngine
{
    struct DescriptorSet;
    struct RenderPasses;

    struct GraphicsPipeline
    {
        std::shared_ptr<RxCore::Pipeline> pipeline;
        VkRenderPass renderPass;
        uint32_t subPass;
    };
#if 0
    struct ShadowPipeline
    {
        std::shared_ptr<RxCore::Pipeline> pipeline;
        VkRenderPass renderPass;
        uint32_t subPass;
    };

    struct OpaquePipeline
    {
        std::shared_ptr<RxCore::Pipeline> pipeline;
        VkRenderPass renderPass;
        uint32_t subPass;
    };

    struct TransparentPipeline
    {
        std::shared_ptr<RxCore::Pipeline> pipeline;
        VkRenderPass renderPass;
        uint32_t subPass;
    };

    struct UiPipeline
    {
        std::shared_ptr<RxCore::Pipeline> pipeline;
        VkRenderPass renderPass;
        uint32_t subPass;
    };
#endif
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
        VkPipelineLayout layout;
        std::vector<VkDescriptorSetLayout> dsls;
        std::vector<uint32_t> counts;
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

        VkPipeline createMaterialPipeline(const MaterialPipelineDetails * mpd,
                                                   const FragmentShader * frag,
                                                   const VertexShader * vert,
                                                   VkPipelineLayout layout,
                                                   VkRenderPass rp,
                                                   uint32_t subpass);

        void createPipelines(ecs::EntityHandle e,
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
