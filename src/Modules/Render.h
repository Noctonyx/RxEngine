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

#if 0
        struct ShadowCaster {};

        struct Pipeline { };
#endif
     

        struct UiRenderCommand
        {
            std::shared_ptr<RxCore::SecondaryCommandBuffer> buf;
        };

        struct OpaqueRenderCommand
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
