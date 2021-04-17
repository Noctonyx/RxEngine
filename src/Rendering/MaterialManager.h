//
// Created by shane on 24/02/2021.
//

#ifndef RX_MATERIALMANAGER_H
#define RX_MATERIALMANAGER_H

#include <memory>
#include <limits>
#include <filesystem>
#include <unordered_map>
#include <SerialisationData.h>
#include "Vulkan/DescriptorSet.hpp"
#include "optick/optick.h"
#include "Defines.h"

namespace RxCore
{
    class DescriptorSet;
};

namespace RxEngine
{
    class Renderer;

    struct MaterialId
    {
        MaterialId() = default;

        explicit MaterialId(uint32_t id)
            : id_(id) {}

        [[nodiscard]] bool valid() const
        {
            return id_ != RX_INVALID_ID;
        }

        [[nodiscard]] uint32_t index() const
        {
            return id_;
        }

        bool operator==(const MaterialId & other) const
        {
            return id_ == other.id_;
        }

    private:
        uint32_t id_ = RX_INVALID_ID;
    };

    struct MaterialBaseId
    {
        MaterialBaseId() = default;

        explicit MaterialBaseId(uint32_t id)
            : id_(id) {}

        [[nodiscard]] bool valid() const
        {
            return id_ != RX_INVALID_ID;
        }

        [[nodiscard]] uint32_t index() const
        {
            return id_;
        }

        bool operator==(const MaterialBaseId & other) const
        {
            return id_ == other.id_;
        }

    private:
        uint32_t id_ = RX_INVALID_ID;
    };

    struct MaterialPipelineId
    {
        MaterialPipelineId() = default;

        explicit MaterialPipelineId(uint32_t id)
            : id_(id) {}

        [[nodiscard]] bool valid() const
        {
            return id_ != RX_INVALID_ID;
        }

        [[nodiscard]] uint32_t index() const
        {
            return id_;
        }

        bool operator==(const MaterialPipelineId & other) const
        {
            return id_ == other.id_;
        }

        bool operator>(const MaterialPipelineId & other) const
        {
            return id_ > other.id_;
        }

        bool operator<(const MaterialPipelineId & other) const
        {
            return id_ < other.id_;
        }

    private:
        uint32_t id_ = RX_INVALID_ID;
    };

    struct TextureId
    {
        TextureId() = default;

        explicit TextureId(uint32_t id)
            : id_(id) {}

        [[nodiscard]] bool valid() const
        {
            return id_ != RX_INVALID_ID;
        }

        [[nodiscard]] uint32_t index() const
        {
            return id_;
        }

        bool operator==(const TextureId & other) const
        {
            return id_ == other.id_;
        }

    private:
        uint32_t id_ = RX_INVALID_ID;
    };

    struct ImageId
    {
        ImageId() = default;

        explicit ImageId(uint32_t id)
            : id_(id) {}

        [[nodiscard]] bool valid() const
        {
            return id_ != RX_INVALID_ID;
        }

        [[nodiscard]] uint32_t index() const
        {
            return id_;
        }

        bool operator==(const ImageId & other) const
        {
            return id_ == other.id_;
        }

    private:
        uint32_t id_ = RX_INVALID_ID;
    };

    struct MaterialPipeline
    {
        vk::Pipeline pipeline;
        vk::ShaderModule vertexShader;
        vk::ShaderModule fragmentShader;

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

        std::string name;
    };

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

        std::string name;
    };

    struct MaterialImage
    {
        std::shared_ptr<RxCore::Image> image;
        std::shared_ptr<RxCore::ImageView> imageView;

        std::string name;
    };

    struct Texture
    {
        RxCore::CombinedSampler combinedSampler;

        std::string name;
    };

    struct MaterialBufferEntry
    {
        DirectX::XMFLOAT4 materialParam1;
        DirectX::XMFLOAT4 materialParam2;
        DirectX::XMFLOAT4 materialParam3;
        DirectX::XMFLOAT4 materialParam4;
        DirectX::XMFLOAT4 materialBaseParam1;
        DirectX::XMFLOAT4 materialBaseParam2;
        DirectX::XMFLOAT4 materialBaseParam3;
        DirectX::XMFLOAT4 materialBaseParam4;
        std::array<TextureId, 4> textures;
    };

    class MaterialManager
    {
        friend class RxEngine::Renderer;

    public:
        MaterialManager(RxCore::Device * device);
        virtual ~MaterialManager();
#if 0
        uint32_t addMaterialPipeline(std::string name, vk::Pipeline pipeline);
        uint32_t addMaterialBase(
            std::string name,
            uint32_t albedoTextureId,
            DirectX::XMFLOAT4 params,
            uint32_t materialPipelineId);
        uint32_t addMaterial(std::string name, uint32_t materialBaseId, DirectX::XMFLOAT4 params);
#endif
        TextureId addTexture(std::string name, std::shared_ptr<RxCore::ImageView> imageView, vk::Sampler sampler);

        void reloadMaterialPipeline(MaterialPipelineId materialPipelineId);
        void reloadMaterial(MaterialId materialId);

        MaterialPipelineId loadMaterialPipeline(std::filesystem::path path);
        MaterialBaseId loadMaterialBase(std::filesystem::path path);
        MaterialId loadMaterial(std::filesystem::path path);
        TextureId loadTexture(std::filesystem::path path);
        ImageId loadImage(std::filesystem::path path);
        uint32_t loadShader(std::filesystem::path path);
        //uint32_t addImage(std::string name, std::shared_ptr<RXCore::Image> image);

        inline const MaterialPipeline & getMaterialPipeline(MaterialPipelineId ix) { return materialPipelines_[ix.index()]; }
        inline const MaterialBase & getMaterialBase(MaterialBaseId ix)
        {
            OPTICK_EVENT()
            return materialBases_[ix.index()];
        }
        inline const Material & getMaterial(MaterialId id)
        {
            OPTICK_EVENT()
            return materials_[id.index()];
        }
        Texture getTexture(TextureId ix) { return textures_[ix.index()]; }
        MaterialImage getImage(ImageId ix) { return images_[ix.index()]; }
        vk::ShaderModule getShader(uint32_t ix) { return shaders_[ix]; }

        //uint32_t getMaterialPipelineCount() const { return materialPipelines_.size(); }
        vk::Sampler createSampler(RxAssets::SamplerData & sd);

        MaterialPipelineId getMaterialPipelineIndexByName(std::string name) const
        {
            return materialPipelineIndex_.at(
                name);
        }
        MaterialId getMaterialIndexByName(std::string name) const { return {materialIndex_.at(name)}; }
        MaterialBaseId getMaterialBaseIndexByName(std::string name) const { return materialBaseIndex_.at(name); }
        TextureId getTextureIndexByName(std::string name) const { return textureIndex_.at(name); }
        uint32_t getShaderIndexByName(std::string name) const { return shaderIndex_.at(name); }

        void updateTextureDescriptor(const std::shared_ptr<RxCore::DescriptorSet> & ds, uint32_t binding);
        void updateMaterialBufferDescriptor(const std::shared_ptr<RxCore::DescriptorSet> & ds, uint32_t binding);

        uint32_t getChangeSequence() const;

        vk::Pipeline createPipeline(
            MaterialPipelineId id,
            vk::PipelineLayout layout,
            vk::RenderPass renderPass,
            uint32_t subPass);

        void ensureOpaqueMaterialPipelinesExist(vk::PipelineLayout layout, vk::RenderPass renderPass, uint32_t subPass);
        void ensureShadowMaterialPipelinesExist(vk::PipelineLayout layout, vk::RenderPass renderPass, uint32_t subPass);

        const std::vector<Material> & getMaterials() const
        {
            return materials_;
        }

        const std::vector<MaterialBase> & getMaterialBases() const
        {
            return materialBases_;
        }

        const std::vector<MaterialPipeline> & getMaterialPipelines() const
        {
            return materialPipelines_;
        }

        void updateMaterial(MaterialId id, const Material & material);
        void materialEditorGui();

        //void createPipelines(vk::PipelineLayout layout);
    protected:
        void rebuildBuffer();
        void updateMaterialBuffer();

        void materialEditor();
        void materialBaseEditor();
        void materialPipelineEditor();
        void texturesEditor();

    private:
        std::vector<MaterialPipeline> materialPipelines_;
        std::vector<MaterialBase> materialBases_;
        std::vector<Material> materials_;
        std::vector<Texture> textures_;
        std::vector<MaterialImage> images_;
        std::vector<vk::ShaderModule> shaders_;

        std::unordered_map<std::string, MaterialPipelineId> materialPipelineIndex_;
        std::unordered_map<std::string, MaterialBaseId> materialBaseIndex_;
        std::unordered_map<std::string, MaterialId> materialIndex_;
        std::unordered_map<std::string, TextureId> textureIndex_;
        std::unordered_map<std::string, ImageId> imageIndex_;
        std::unordered_map<std::string, uint32_t> shaderIndex_;

        std::shared_ptr<RxCore::Buffer> materialDataBuffer_;

        uint32_t changeSequence_{};

        RxCore::Device * device_;

        std::shared_ptr<RxCore::Buffer> materialBuffer_;
        uint32_t materialBufferSequence_;
        void updateMaterialBase(MaterialBaseId id, const MaterialBase & material);
        void reloadMaterialBase(MaterialBaseId id);
    };
}
#endif //RX_MATERIALMANAGER_H
