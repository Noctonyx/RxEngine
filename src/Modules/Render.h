#pragma once

#include <RxECS.h>
#include "./Mesh.h"
#include "DirectXCollision.h"
#include "SerialisationData.h"
#include "Vulkan/CommandBuffer.hpp"

namespace RxCore
{
    struct DescriptorPoolTemplate;
    class Shader;
    class Pipeline;
}

namespace RxEngine
{
    struct Render
    {
        using MaterialId = ecs::entity_t;
        //        using MaterialBaseId = flecs::entity;
        //using MaterialPipelineId = flecs::entity;
        using TextureId = ecs::entity_t;
        using ImageId = ecs::entity_t;

        struct ShadowCaster {};

        struct Pipeline { };

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
            //vk::Pipeline pipeline;
            //vk::ShaderModule vertexShader;
            //vk::ShaderModule fragmentShader;

            //flecs::entity_view vertexShader;
            //flecs::entity_view fragmentShader;
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

            //std::string name;
        };

        struct UiRenderCommand
        {
            std::shared_ptr<RxCore::SecondaryCommandBuffer> buf;
        };

#if 0
        struct ShaderModule
        {
            std::shared_ptr<RxCore::Shader> shader;
            //vk::ShaderModule module{};
            std::string shaderAssetName{};
            //vk::Device device{};
        };
#endif
#if 0
        struct MaterialTexture
        {
            TextureId texture1Id;
            TextureId texture2Id;
            TextureId texture3Id;
            TextureId texture4Id;
        };
#endif
#if 0
        struct MaterialBase
        {
            MaterialPipelineId opaquePipelineId;
            MaterialPipelineId shadowPipelineId;
            MaterialPipelineId transparentPipelineId;
            TextureId texture1Id;
            TextureId texture2Id;
            TextureId texture3Id;
            TextureId texture4Id;
            DirectX::XMFLOAT4 param1;
            DirectX::XMFLOAT4 param2;
            DirectX::XMFLOAT4 param3;
            DirectX::XMFLOAT4 param4;

            std::string name;
        };

        struct Material
        {
            MaterialBaseId materialBaseId;
            DirectX::XMFLOAT4 param1;
            DirectX::XMFLOAT4 param2;
            DirectX::XMFLOAT4 param3;
            DirectX::XMFLOAT4 param4;

            //std::string name;
        };
#endif

        struct MaterialImage
        {
            std::shared_ptr<RxCore::Image> image;
            std::shared_ptr<RxCore::ImageView> imageView;

            //std::string name;
        };
#if 0
        struct Texture
        {
            RxCore::CombinedSampler combinedSampler;

            //std::string name;
        };
#endif
        struct MaterialSampler
        {
            vk::Sampler sampler;
            uint32_t sequence;
        };
#if 0
        struct MaterialTexture
        {
            RxCore::CombinedSampler combinedSampler;
            std::shared_ptr<RxCore::Image> image;
            std::shared_ptr<RxCore::ImageView> imageView;
            uint32_t shaderId{9999};
        };
#endif
        struct Material
        {
            std::array<ecs::entity_t, 4> materialTextures{0, 0, 0, 0};
        };

        struct MaterialBufferDataEntry
        {
            std::array<uint32_t, 4> textures;
        };

        struct MaterialBufferData
        {
            std::vector<MaterialBufferDataEntry> entries;
        };

        struct RenderMesh
        {
            //MaterialId materialId;
            //uint8_t lodCount;
            //std::array<std::pair<Mesh::MeshBundleId, float>, 4> lods;
            //uint8_t selectedLod;
            DirectX::BoundingBox bounds;
        };

        static void registerPlugin(ecs::World * world)
        {
#if 0
            ecs.module<Render>();

           

            //ecs.component<Material>();
            //ecs.component<MaterialBase>();
            //ecs.component<MaterialPipeline>();
            auto c = ecs.component<Pipeline>();
            ecs.component<OpaquePipeline>().add(flecs::IsA, c);
            ecs.component<ShadowPipeline>().add(flecs::IsA, c);
            ecs.component<TransparentPipeline>().add(flecs::IsA, c);
            ecs.component<UiPipeline>().add(flecs::IsA, c);

            ecs.component<MaterialPipelineDetails>();
            ecs.component<MaterialImage>();
            //ecs.component<Texture>();
            ecs.component<Material>();
            ecs.component<RenderMesh>();
            ecs.component<VertexShader>();
            ecs.component<FragmentShader>();
            ecs.component<MaterialSampler>();
            ecs.component<MaterialBufferData>();
#endif
            world->setSingleton<MaterialBufferData>({.entries = {}});
            //ecs.set<MaterialBufferData>({ .entries = {} });
        };
    };
}
