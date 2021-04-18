//
// Created by shane on 24/02/2021.
//

#include <algorithm>
#include <Vulkan/Device.h>
#include <Vulkan/Image.hpp>
#include <SerialisationData.h>
#include <Loader.h>
#include "MaterialManager.h"
#include <imgui.h>

namespace RxEngine
{
#if 0
    uint32_t MaterialManager::addMaterialPipeline(std::string name, vk::Pipeline pipeline)
    {
        if (materialPipelineIndex_.contains(name)) {
            return materialPipelineIndex_.at(name);
        }

        auto ix = static_cast<uint32_t>(materialPipelines_.size());

        materialPipelines_.push_back(MaterialPipeline{pipeline});
        materialPipelineIndex_.emplace(name, ix);

        changeSequence_++;
        return ix;
    }

    uint32_t MaterialManager::addMaterialBase(
        std::string name,
        uint32_t albedoTextureId,
        DirectX::XMFLOAT4 params,
        uint32_t materialPipelineId)
    {
        if (materialBaseIndex_.contains(name)) {
            return materialBaseIndex_.at(name);
        }

        auto ix = static_cast<uint32_t>(materialBases_.size());

        materialBases_.push_back(MaterialBase{materialPipelineId, albedoTextureId, params});
        materialBaseIndex_.emplace(name, ix);

        changeSequence_++;
        return ix;
    }

    uint32_t MaterialManager::addMaterial(std::string name, uint32_t materialBaseId, DirectX::XMFLOAT4 params)
    {
        if (materialIndex_.contains(name)) {
            return materialIndex_.at(name);
        }

        auto ix = static_cast<uint32_t>(materials_.size());

        materials_.push_back(Material{materialBaseId, params});
        materialIndex_.emplace(name, ix);

        changeSequence_++;
        return ix;
    }
#endif
    TextureId MaterialManager::addTexture(
        std::string name,
        std::shared_ptr<RxCore::ImageView> imageView,
        vk::Sampler sampler)
    {
        if (textureIndex_.contains(name)) {
            return textureIndex_.at(name);
        }

        auto ix = static_cast<uint32_t>(textures_.size());

        textures_.push_back({ RxCore::CombinedSampler{sampler, std::move(imageView)}, name });
        textureIndex_.emplace(name, ix);

        changeSequence_++;
        return TextureId{ix};
    }

    ImageId MaterialManager::loadImage(std::filesystem::path path)
    {
        if (imageIndex_.contains(path.generic_string())) {
            return imageIndex_.at(path.generic_string());
        }

        auto ix = static_cast<uint32_t>(textures_.size());

        RxAssets::ImageData id{};
        RxAssets::Loader::loadImage(id, path);

        auto image = RxCore::iVulkan()->createImage(
            id.imType == RxAssets::eBC7 ? vk::Format::eBc7UnormBlock
                                        : vk::Format::eR8G8B8A8Unorm,
            vk::Extent3D{id.width, id.height, 1},
            static_cast<uint32_t>(id.mipLevels.size()),
            1,
            vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
            vk::ImageType::e2D);

        auto iv =
            image->createImageView(vk::ImageViewType::e2D, vk::ImageAspectFlagBits::eColor);

        for (uint32_t j = 0; j < id.mipLevels.size(); j++) {
            const auto staging_buffer = RxCore::iVulkan()->createStagingBuffer(
                id.mipLevels[j].bytes.size(), id.mipLevels[j].bytes.data());

            RxCore::iVulkan()->transferBufferToImage(
                staging_buffer,
                image,
                vk::Extent3D(id.mipLevels[j].width, id.mipLevels[j].height, 1),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                1,
                0,
                j);
        }

        images_.push_back({ image, iv, path.generic_string() });
        imageIndex_.emplace(path.generic_string(), ix);

        changeSequence_++;
        return ImageId{ix};
    }

    void MaterialManager::updateTextureDescriptor(const std::shared_ptr<RxCore::DescriptorSet> & ds, uint32_t binding)
    {
        std::vector<RxCore::CombinedSampler> samplers;

        std::transform(
            textures_.begin(), textures_.end(), std::back_inserter(samplers), [](auto & x)
            {
                return x.combinedSampler;
            }
        );

        ds->updateDescriptor(binding, vk::DescriptorType::eCombinedImageSampler, samplers);
    }

    void MaterialManager::rebuildBuffer()
    {

    }

    MaterialManager::~MaterialManager()
    {
        for (auto & sm: shaders_) {
            device_->getDevice().destroyShaderModule(sm);
        }
        for (auto & mp: materialPipelines_) {
            device_->getDevice().destroyPipeline(mp.pipeline);
        }
        textures_.clear();
        materials_.clear();
        materialBases_.clear();
        materialPipelines_.clear();
        images_.clear();

        materialPipelineIndex_.clear();
        shaderIndex_.clear();
        textureIndex_.clear();
        materialIndex_.clear();
        materialBaseIndex_.clear();
        imageIndex_.clear();

        materialDataBuffer_.reset();
    }

    uint32_t MaterialManager::getChangeSequence() const
    {
        return changeSequence_;
    }

    MaterialManager::MaterialManager(RxCore::Device * device)
        : device_(device) {}

    vk::Sampler MaterialManager::createSampler(RxAssets::SamplerData & sd)
    {
        vk::SamplerCreateInfo sci;

        sci.setMinFilter(static_cast<vk::Filter>(sd.minFilter));
        sci.setMagFilter(static_cast<vk::Filter>(sd.magFilter));
        sci.setMipmapMode(static_cast<vk::SamplerMipmapMode>(sd.mipMapMode));
        sci.setAddressModeU(static_cast<vk::SamplerAddressMode>(sd.addressU));
        sci.setAddressModeV(static_cast<vk::SamplerAddressMode>(sd.addressV));
        sci.setAddressModeW(static_cast<vk::SamplerAddressMode>(sd.addressW));
        sci.setMipLodBias(sd.mipLodBias);
        sci.setAnisotropyEnable(sd.anisotropy);
        sci.setMaxAnisotropy(sd.maxAnisotropy);
        sci.setMinLod(sd.minLod);
        sci.setMaxLod(sd.maxLod);

        return device_->createSampler(sci);
    }

    MaterialId MaterialManager::loadMaterial(std::filesystem::path path)
    {
        if (path == "") {
            return {};
        }

        if (materialIndex_.contains(path.generic_string())) {
            return materialIndex_.at(path.generic_string());
        }

        RxAssets::MaterialData2 md{};
        RxAssets::Loader::loadMaterial(md, path);

        MaterialBaseId material_base_id = loadMaterialBase(md.materialBaseName);

        auto ix = static_cast<uint32_t>(materials_.size());
        DirectX::XMFLOAT4 param1(md.val1[0], md.val1[1], md.val1[2], md.val1[3]);
        DirectX::XMFLOAT4 param2(md.val2[0], md.val2[1], md.val2[2], md.val2[3]);
        DirectX::XMFLOAT4 param3(md.val3[0], md.val3[1], md.val3[2], md.val3[3]);
        DirectX::XMFLOAT4 param4(md.val4[0], md.val4[1], md.val4[2], md.val4[3]);

        materials_.push_back({ material_base_id, param1, param2, param3, param4, path.generic_string() });
        materialIndex_.emplace(path.generic_string(), ix);

        changeSequence_++;

        return MaterialId{ix};
    }

    void MaterialManager::reloadMaterial(MaterialId materialId)
    {
        auto & m = materials_[materialId.index()];

        RxAssets::MaterialData2 md{};
        RxAssets::Loader::loadMaterial(md, m.name);

        MaterialBaseId material_base_id = loadMaterialBase(md.materialBaseName);

        DirectX::XMFLOAT4 param1(md.val1[0], md.val1[1], md.val1[2], md.val1[3]);
        DirectX::XMFLOAT4 param2(md.val2[0], md.val2[1], md.val2[2], md.val2[3]);
        DirectX::XMFLOAT4 param3(md.val3[0], md.val3[1], md.val3[2], md.val3[3]);
        DirectX::XMFLOAT4 param4(md.val4[0], md.val4[1], md.val4[2], md.val4[3]);

        m.materialBaseId = material_base_id;
        m.param1 = param1;
        m.param2 = param2;
        m.param3 = param3;
        m.param4 = param4;

        changeSequence_++;
        updateMaterialBuffer();
    }

    MaterialBaseId MaterialManager::loadMaterialBase(std::filesystem::path path)
    {
        if (path == "") {
            return {};
        }

        if (materialBaseIndex_.contains(path.generic_string())) {
            return materialBaseIndex_.at(path.generic_string());
        }

        RxAssets::MaterialBaseData mbd{};
        RxAssets::Loader::loadMaterialBase(mbd, path);

        MaterialPipelineId opaque_material_pipe_ix = loadMaterialPipeline(mbd.opaquePipelineName);
        MaterialPipelineId shadow_material_pipe_ix = loadMaterialPipeline(mbd.shadowPipelineName);
        MaterialPipelineId transparent_material_pipe_ix = loadMaterialPipeline(mbd.transparentPipelineName);

        TextureId texture1_ix = loadTexture(mbd.texture1);
        TextureId texture2_ix = loadTexture(mbd.texture2);
        TextureId texture3_ix = loadTexture(mbd.texture3);
        TextureId texture4_ix = loadTexture(mbd.texture4);

        auto ix = static_cast<uint32_t>(materials_.size());
        DirectX::XMFLOAT4 param1(mbd.val1[0], mbd.val1[1], mbd.val1[2], mbd.val1[3]);
        DirectX::XMFLOAT4 param2(mbd.val2[0], mbd.val2[1], mbd.val2[2], mbd.val2[3]);
        DirectX::XMFLOAT4 param3(mbd.val3[0], mbd.val3[1], mbd.val3[2], mbd.val3[3]);
        DirectX::XMFLOAT4 param4(mbd.val4[0], mbd.val4[1], mbd.val4[2], mbd.val4[3]);

        materialBases_.push_back({
            opaque_material_pipe_ix,
            shadow_material_pipe_ix,
            transparent_material_pipe_ix,
            texture1_ix,
            texture2_ix,
            texture3_ix,
            texture4_ix,
            param1,
            param2,
            param3,
            param4,
            path.generic_string()
            }
        );
        materialBaseIndex_.emplace(path.generic_string(), ix);

        changeSequence_++;

        return MaterialBaseId{ix};
    }

    void MaterialManager::reloadMaterialBase(MaterialBaseId id)
    {
        auto & m = materialBases_[id.index()];

        RxAssets::MaterialBaseData mbd{};
        RxAssets::Loader::loadMaterialBase(mbd, m.name);

        MaterialPipelineId opaque_material_pipe_ix = loadMaterialPipeline(mbd.opaquePipelineName);
        MaterialPipelineId shadow_material_pipe_ix = loadMaterialPipeline(mbd.shadowPipelineName);
        MaterialPipelineId transparent_material_pipe_ix = loadMaterialPipeline(mbd.transparentPipelineName);

        TextureId texture1_ix = loadTexture(mbd.texture1);
        TextureId texture2_ix = loadTexture(mbd.texture2);
        TextureId texture3_ix = loadTexture(mbd.texture3);
        TextureId texture4_ix = loadTexture(mbd.texture4);

        DirectX::XMFLOAT4 param1(mbd.val1[0], mbd.val1[1], mbd.val1[2], mbd.val1[3]);
        DirectX::XMFLOAT4 param2(mbd.val2[0], mbd.val2[1], mbd.val2[2], mbd.val2[3]);
        DirectX::XMFLOAT4 param3(mbd.val3[0], mbd.val3[1], mbd.val3[2], mbd.val3[3]);
        DirectX::XMFLOAT4 param4(mbd.val4[0], mbd.val4[1], mbd.val4[2], mbd.val4[3]);

        m.opaquePipelineId = opaque_material_pipe_ix;
        m.shadowPipelineId = shadow_material_pipe_ix;
        m.transparentPipelineId = transparent_material_pipe_ix;
        m.texture1Id = texture1_ix;
        m.texture2Id = texture2_ix;
        m.texture3Id = texture3_ix;
        m.texture4Id = texture4_ix;
        m.param1 = param1;
        m.param2 = param2;
        m.param3 = param3;
        m.param4 = param4;

        changeSequence_++;
    }

    TextureId MaterialManager::loadTexture(std::filesystem::path path)
    {
        if (path == "") {
            return {};
        }

        if (textureIndex_.contains(path.generic_string())) {
            return textureIndex_.at(path.generic_string());
        }

        RxAssets::TextureData td{};
        RxAssets::Loader::loadTexture(td, path);

        ImageId image_ix = loadImage(td.imageName);
        auto s = createSampler(td.sampler);

        auto im = getImage(image_ix);

        uint32_t ix = static_cast<uint32_t>(materials_.size());
        textures_.push_back({RxCore::CombinedSampler{s, im.imageView}, path.generic_string()});
        textureIndex_.emplace(path.generic_string(), ix);

        return TextureId{ix};
    }

    MaterialPipelineId MaterialManager::loadMaterialPipeline(std::filesystem::path path)
    {
        if (path == "") {
            return {};
        }

        if (materialPipelineIndex_.contains(path.generic_string())) {
            return materialPipelineIndex_.at(path.generic_string());
        }

        RxAssets::MaterialPipelineData mpd{};
        RxAssets::Loader::loadMaterialPipeline(mpd, path);

        auto ix = static_cast<uint32_t>(materialPipelines_.size());

        auto & np = materialPipelines_.emplace_back();
        np.vertexShader = getShader(loadShader(mpd.vertexShader));
        np.fragmentShader = getShader(loadShader(mpd.fragmentShader));

        np.lineWidth = mpd.lineWidth;
        np.fillMode = mpd.fillMode;
        np.depthClamp = mpd.depthClamp;
        np.cullMode = mpd.cullMode;
        np.frontFace = mpd.frontFace;

        np.depthTestEnable = mpd.depthTestEnable;
        np.depthWriteEnable = mpd.depthWriteEnable;
        np.depthCompareOp = mpd.depthCompareOp;

        np.stencilTest = mpd.stencilTest;
        np.minDepth = mpd.minDepth;
        np.maxDepth = mpd.maxDepth;

        np.pipeline = nullptr;

        np.stage = mpd.stage;

        np.name = mpd.name;

        np.blends.resize(mpd.blends.size());
        np.inputs.resize(mpd.inputs.size());
        std::copy(mpd.blends.begin(), mpd.blends.end(), np.blends.begin());
        std::copy(mpd.inputs.begin(), mpd.inputs.end(), np.inputs.begin());

        materialPipelineIndex_.emplace(path.generic_string(), ix);

        return MaterialPipelineId{ix};
    }

    void MaterialManager::reloadMaterialPipeline(MaterialPipelineId materialPipelineId)
    {
        auto & np = materialPipelines_[materialPipelineId.index()];

        RxAssets::MaterialPipelineData mpd{};
        RxAssets::Loader::loadMaterialPipeline(mpd, np.name);

        np.vertexShader = getShader(loadShader(mpd.vertexShader));
        np.fragmentShader = getShader(loadShader(mpd.fragmentShader));

        np.lineWidth = mpd.lineWidth;
        np.fillMode = mpd.fillMode;
        np.depthClamp = mpd.depthClamp;
        np.cullMode = mpd.cullMode;
        np.frontFace = mpd.frontFace;

        np.depthTestEnable = mpd.depthTestEnable;
        np.depthWriteEnable = mpd.depthWriteEnable;
        np.depthCompareOp = mpd.depthCompareOp;

        np.stencilTest = mpd.stencilTest;
        np.minDepth = mpd.minDepth;
        np.maxDepth = mpd.maxDepth;

        //np.pipeline = nullptr;

        np.stage = mpd.stage;

        np.name = mpd.name;

        np.blends.resize(mpd.blends.size());
        np.inputs.resize(mpd.inputs.size());
        std::copy(mpd.blends.begin(), mpd.blends.end(), np.blends.begin());
        std::copy(mpd.inputs.begin(), mpd.inputs.end(), np.inputs.begin());

        if (np.pipeline) {
            device_->getDevice().waitIdle();
            device_->getDevice().destroyPipeline(np.pipeline);
            np.pipeline = nullptr;
        }
    }

    vk::Pipeline MaterialManager::createPipeline(
        MaterialPipelineId id,
        vk::PipelineLayout layout,
        vk::RenderPass renderPass,
        uint32_t subPass)
    {
        auto mp = getMaterialPipeline(id);

        vk::GraphicsPipelineCreateInfo gpci;
        vk::PipelineDynamicStateCreateInfo pdsci;
        vk::PipelineColorBlendStateCreateInfo pcbsci;
        vk::PipelineDepthStencilStateCreateInfo pdssci;
        vk::PipelineMultisampleStateCreateInfo pmsci;
        vk::PipelineRasterizationStateCreateInfo prsci;
        vk::PipelineViewportStateCreateInfo pvsci;
        vk::PipelineInputAssemblyStateCreateInfo piasci;
        vk::PipelineVertexInputStateCreateInfo pvisci;
        //std::shared_ptr<PipelineLayout> pipelineLayout_;
        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
        std::vector<vk::PipelineColorBlendAttachmentState> attachments;
        std::vector<vk::DynamicState> dynamicStates;

        piasci.setTopology(vk::PrimitiveTopology::eTriangleList)
              .setPrimitiveRestartEnable(false);

        pvsci.setViewportCount(1)
             .setPViewports(nullptr)
             .setScissorCount(1);

        prsci.setLineWidth(mp.lineWidth)
             .setPolygonMode(static_cast<vk::PolygonMode>(mp.fillMode))
             .setDepthClampEnable(mp.depthClamp)
             .setRasterizerDiscardEnable(false)
             .setCullMode(static_cast<vk::CullModeFlagBits>(mp.cullMode))
             .setFrontFace(static_cast<vk::FrontFace>(mp.frontFace));

        pmsci.setRasterizationSamples(vk::SampleCountFlagBits::e1);

        shaderStages.push_back(
            vk::PipelineShaderStageCreateInfo{
                {},
                vk::ShaderStageFlagBits::eVertex,
                mp.vertexShader,
                "main"
            }
        );
        shaderStages.push_back(
            vk::PipelineShaderStageCreateInfo{
                {},
                vk::ShaderStageFlagBits::eFragment,
                mp.fragmentShader,
                "main"
            }
        );
#if 0
        vk::PipelineShaderStageCreateInfo & st = shaderStages.emplace_back();
        st.setStage(vk::ShaderStageFlagBits::eVertex).setModule(mp.vertexShader);
        st.setPName("main");
        st = shaderStages.emplace_back();
        st.setStage(vk::ShaderStageFlagBits::eFragment).setModule(mp.fragmentShader);
        st.setPName("main");
#endif
        for (auto & mpa: mp.blends) {
            auto & at = attachments.emplace_back();
            at.setColorWriteMask(
                  vk::ColorComponentFlagBits::eA |
                  vk::ColorComponentFlagBits::eR |
                  vk::ColorComponentFlagBits::eG |
                  vk::ColorComponentFlagBits::eB)
              .setBlendEnable(mpa.enable)
              .setSrcColorBlendFactor(static_cast<vk::BlendFactor>(mpa.sourceFactor))
              .setDstColorBlendFactor(static_cast<vk::BlendFactor>(mpa.destFactor))
              .setColorBlendOp(static_cast<vk::BlendOp>(mpa.colorBlendOp))
              .setSrcAlphaBlendFactor(static_cast<vk::BlendFactor>(mpa.sourceAlphaFactor))
              .setDstAlphaBlendFactor(static_cast<vk::BlendFactor>(mpa.destAlphaFactor))
              .setAlphaBlendOp(static_cast<vk::BlendOp>(mpa.alphaBlendOp));
        }

        pdssci.setDepthTestEnable(mp.depthWriteEnable)
              .setDepthWriteEnable(mp.depthTestEnable)
              .setDepthCompareOp(static_cast<vk::CompareOp>(mp.depthCompareOp))
              .setDepthBoundsTestEnable(false)
              .setStencilTestEnable(mp.stencilTest)
              .setFront({vk::StencilOp::eKeep, vk::StencilOp::eKeep})
              .setBack({vk::StencilOp::eKeep, vk::StencilOp::eKeep})
              .setMinDepthBounds(mp.minDepth)
              .setMaxDepthBounds(mp.maxDepth);

        dynamicStates.push_back(vk::DynamicState::eViewport);
        dynamicStates.push_back(vk::DynamicState::eScissor);

        gpci.setPInputAssemblyState(&piasci)
            .setPViewportState(&pvsci)
            .setPRasterizationState(&prsci)
            .setPMultisampleState(&pmsci)
            .setPDepthStencilState(&pdssci)
            .setPColorBlendState(&pcbsci)
            .setPVertexInputState(&pvisci)
            .setPDynamicState(&pdsci)
            .setLayout(layout)
            .setStages(shaderStages)
            .setRenderPass(renderPass)
            .setSubpass(subPass);

        std::vector<vk::VertexInputBindingDescription> bindings;
        std::vector<vk::VertexInputAttributeDescription> attributes;

        if (mp.inputs.size() > 0) {
            uint32_t offset = 0;
            uint32_t loc = 0;

            for (auto & i: mp.inputs) {
                if (i.inputType == RxAssets::MaterialPipelineInputType::eFloat) {
                    switch (i.count) {
                        case 1:
                            attributes.emplace_back(loc++, 0, vk::Format::eR32Sfloat, offset);
                            offset += 4;
                            break;
                        case 2:
                            attributes.emplace_back(loc++, 0, vk::Format::eR32G32Sfloat, offset);
                            offset += 8;
                            break;
                        case 3:
                            attributes.emplace_back(loc++, 0, vk::Format::eR32G32B32Sfloat, offset);
                            offset += 12;
                            break;
                        case 4:
                            attributes.emplace_back(loc++, 0, vk::Format::eR32G32B32A32Sfloat, offset);
                            offset += 16;
                            break;
                    }
                }
                if (i.inputType == RxAssets::MaterialPipelineInputType::eByte) {
                    switch (i.count) {
                        case 1:
                            attributes.emplace_back(loc++, 0, vk::Format::eR8Unorm, offset);
                            offset += 1;
                            break;
                        case 2:
                            attributes.emplace_back(loc++, 0, vk::Format::eR8G8Unorm, offset);
                            offset += 2;
                            break;
                        case 3:
                            attributes.emplace_back(loc++, 0, vk::Format::eR8G8B8Unorm, offset);
                            offset += 3;
                            break;
                        case 4:
                            attributes.emplace_back(loc++, 0, vk::Format::eR8G8B8A8Unorm, offset);
                            offset += 4;
                            break;
                    }
                }
            }

            bindings.emplace_back(0, offset, vk::VertexInputRate::eVertex);
            pvisci.setVertexBindingDescriptions(bindings).setVertexAttributeDescriptions(attributes);

        }

        pcbsci.setAttachments(attachments);
        pdsci.setDynamicStates(dynamicStates);

        auto rv = device_->getDevice().createGraphicsPipeline(nullptr, gpci);
        assert(rv.result == vk::Result::eSuccess);

        return rv.value;
    }

    uint32_t MaterialManager::loadShader(std::filesystem::path path)
    {
        if (path == "") {
            return RX_INVALID_ID;
        }

        if (shaderIndex_.contains(path.generic_string())) {
            return shaderIndex_.at(path.generic_string());
        }

        RxAssets::ShaderData sd{};
        RxAssets::Loader::loadShader(sd, path);
        auto sm = RxCore::Device::VkDevice().createShaderModule(
            {{}, uint32_t(sd.bytes.size() * 4), (uint32_t *) sd.bytes.data()});

        uint32_t ix = static_cast<uint32_t>(shaders_.size());
        shaders_.push_back(sm);

        shaderIndex_.emplace(path.generic_string(), ix);

        return ix;
    }

    void MaterialManager::updateMaterialBuffer()
    {
        if (materialBufferSequence_ == changeSequence_ && materialBuffer_) {
            return;
        }

        std::vector<MaterialBufferEntry> buf;

        for (auto & m: materials_) {
            auto & x = buf.emplace_back();

            auto & mb = materialBases_[m.materialBaseId.index()];

            x.materialParam1 = m.param1;
            x.materialParam2 = m.param2;
            x.materialParam3 = m.param3;
            x.materialParam4 = m.param4;
            x.materialBaseParam1 = mb.param1;
            x.materialBaseParam2 = mb.param2;
            x.materialBaseParam3 = mb.param3;
            x.materialBaseParam4 = mb.param4;
            x.textures[0] = mb.texture1Id;
            x.textures[1] = mb.texture2Id;
            x.textures[2] = mb.texture3Id;
            x.textures[3] = mb.texture4Id;
        }

        materialBuffer_ = device_->createBuffer(
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
            VMA_MEMORY_USAGE_GPU_ONLY,
            static_cast<uint32_t>(buf.size() * sizeof(MaterialBufferEntry)), buf.data());

        materialBufferSequence_ = changeSequence_;
    }

    void MaterialManager::updateMaterialBufferDescriptor(
        const std::shared_ptr<RxCore::DescriptorSet> & ds,
        uint32_t binding)
    {
        updateMaterialBuffer();

        ds->updateDescriptor(binding, vk::DescriptorType::eStorageBuffer, materialBuffer_);
    }

    void MaterialManager::ensureOpaqueMaterialPipelinesExist(
        vk::PipelineLayout layout,
        vk::RenderPass renderPass,
        uint32_t subPass)
    {
        for (uint32_t i = 0; i < materialPipelines_.size(); i++) { ;
            if (!materialPipelines_[i].pipeline) {
                if (materialPipelines_[i].stage == RxAssets::PipelineRenderStage::Opaque) {
                    materialPipelines_[i].pipeline = createPipeline(
                        MaterialPipelineId{i},
                        layout,
                        renderPass,
                        subPass);
                }
            }
        }
    }

    void MaterialManager::ensureShadowMaterialPipelinesExist(
        vk::PipelineLayout layout,
        vk::RenderPass renderPass,
        uint32_t subPass)
    {
        for (uint32_t i = 0; i < materialPipelines_.size(); i++) { ;
            if (!materialPipelines_[i].pipeline) {
                if (materialPipelines_[i].stage == RxAssets::PipelineRenderStage::Shadow) {
                    materialPipelines_[i].pipeline = createPipeline(
                        MaterialPipelineId{i},
                        layout,
                        renderPass,
                        subPass);
                }
            }
        }
    }

    void MaterialManager::updateMaterial(MaterialId id, const Material & material)
    {
        Material & mo = materials_[id.index()];
        mo.param1 = material.param1;
        mo.param2 = material.param2;
        mo.param3 = material.param3;
        mo.param4 = material.param4;

        updateMaterialBuffer();
    }

    void MaterialManager::updateMaterialBase(MaterialBaseId id, const MaterialBase & material)
    {
        MaterialBase & mo = materialBases_[id.index()];
        mo.param1 = material.param1;
        mo.param2 = material.param2;
        mo.param3 = material.param3;
        mo.param4 = material.param4;

        updateMaterialBuffer();
    }

    void MaterialManager::materialEditorGui()
    {
        if (ImGui::BeginTabBar("MatManager")) {

            if (ImGui::BeginTabItem("Materials")) {
                materialEditor();

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Material Bases")) {
                materialBaseEditor();

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Material Pipelines")) {
                materialPipelineEditor();

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Textures")) {
                texturesEditor();

                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }

    void MaterialManager::texturesEditor()
    {
        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit;
        static uint32_t selectedTexture = 0;
        if (ImGui::BeginTable("Textures", 3, flags)) {

            ImGui::TableSetupColumn("ID");
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Action");
            ImGui::TableHeadersRow();

            for (uint32_t row = 0; row < textures_.size(); row++) {
                auto mm = textures_[row];

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%d", row);
                ImGui::TableNextColumn();
                if (ImGui::Selectable(
                    mm.name.c_str(),
                    selectedTexture == row)) {
                    selectedTexture = row;
                }
                ImGui::TableNextColumn();
                //ImGui::TableSetColumnIndex(column);
                ImGui::Button("Reload");
            }
            ImGui::EndTable();
        }
    }

    void MaterialManager::materialPipelineEditor()
    {
        ImGui::PushID("MaterialPipelineEditor");

        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit;
        static uint32_t selectedMaterialPipeline = 0;

        if (ImGui::BeginTable("Pipelines", 3, flags)) {

            ImGui::TableSetupColumn("ID");
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Action");
            ImGui::TableHeadersRow();

            for (uint32_t row = 0; row < materialPipelines_.size(); row++) {
                auto mm = materialPipelines_[row];

                ImGui::PushID(row);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%d", row);
                ImGui::TableNextColumn();
                if (ImGui::Selectable(
                    mm.name.c_str(),
                    selectedMaterialPipeline == row)) {
                    selectedMaterialPipeline = row;
                }
                ImGui::TableNextColumn();
                //ImGui::TableSetColumnIndex(column);
                if (ImGui::Button("Reload")) {
                    reloadMaterialPipeline(MaterialPipelineId{row});
                }
                ImGui::SameLine();
                ImGui::Button("Save");
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::PopID();
    }

    void MaterialManager::materialBaseEditor()
    {
        ImGui::PushID("MaterialBaseEditor");

        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit;
        static uint32_t selectedMaterialBase = 0;

        if (ImGui::BeginTable("MaterialBases", 3, flags)) {

            ImGui::TableSetupColumn("ID");
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Action");
            ImGui::TableHeadersRow();

            for (uint32_t row = 0; row < materialBases_.size(); row++) {
                auto mm = materialBases_[row];

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%d", row);
                ImGui::TableNextColumn();
                if (ImGui::Selectable(
                    mm.name.c_str(),
                    selectedMaterialBase == row)) {
                    selectedMaterialBase = row;
                }
                ImGui::TableNextColumn();
                if (ImGui::Button("Reload")) {
                    reloadMaterialBase(MaterialBaseId{row});
                }
                ImGui::SameLine();
                ImGui::Button("Reload All");
                ImGui::SameLine();
                ImGui::Button("Save");
            }
            ImGui::EndTable();
        }

        if (selectedMaterialBase < materials_.size()) {
            auto mm = materialBases_[selectedMaterialBase];

            ImGui::PushID(selectedMaterialBase);
            std::array<float, 4> param1 = {mm.param1.x, mm.param1.y, mm.param1.z, mm.param1.w};
            std::array<float, 4> param2 = {mm.param2.x, mm.param2.y, mm.param2.z, mm.param2.w};
            std::array<float, 4> param3 = {mm.param3.x, mm.param3.y, mm.param3.z, mm.param3.w};
            std::array<float, 4> param4 = {mm.param4.x, mm.param4.y, mm.param4.z, mm.param4.w};

            if (ImGui::BeginTable(
                "SelectedMaterialBase",
                2,
                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Material Name");
                ImGui::TableNextColumn();
                ImGui::Text("%s", mm.name.c_str());

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Opaque Material Pipeline");
                ImGui::TableNextColumn();
                if (mm.opaquePipelineId.valid()) {
                    ImGui::Text("%s", materialPipelines_[mm.opaquePipelineId.index()].name.c_str());
                } else {
                    ImGui::Text("");
                }

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Shadow Material Pipeline");
                ImGui::TableNextColumn();
                if (mm.shadowPipelineId.valid()) {
                    ImGui::Text("%s", materialPipelines_[mm.shadowPipelineId.index()].name.c_str());
                } else {
                    ImGui::Text("");
                }

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Transparent Material Pipeline");
                ImGui::TableNextColumn();
                if (mm.transparentPipelineId.valid()) {
                    ImGui::Text("%s", materialPipelines_[mm.transparentPipelineId.index()].name.c_str());
                } else {
                    ImGui::Text("");
                }

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Param 1");
                ImGui::TableNextColumn();
                ImGui::PushID(1);
                if (ImGui::DragFloat4("", param1.data())) {
                    mm.param1 = DirectX::XMFLOAT4(param1[0], param1[1], param1[2], param1[3]);
                    updateMaterialBase(MaterialBaseId{selectedMaterialBase}, mm);
                }
                ImGui::PopID();

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Param 2");
                ImGui::TableNextColumn();
                ImGui::PushID(2);
                if (ImGui::DragFloat4("", param2.data())) {
                    mm.param2 = DirectX::XMFLOAT4(param2[0], param2[1], param2[2], param2[3]);
                    updateMaterialBase(MaterialBaseId{selectedMaterialBase}, mm);
                }
                ImGui::PopID();

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Param 3");
                ImGui::TableNextColumn();
                ImGui::PushID(3);
                if (ImGui::DragFloat4("", param3.data())) {
                    mm.param3 = DirectX::XMFLOAT4(param3[0], param3[1], param3[2], param3[3]);
                    updateMaterialBase(MaterialBaseId{selectedMaterialBase}, mm);
                }
                ImGui::PopID();

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Param 4");
                ImGui::TableNextColumn();
                ImGui::PushID(4);
                if (ImGui::DragFloat4("", param4.data())) {
                    mm.param4 = DirectX::XMFLOAT4(param4[0], param4[1], param4[2], param4[3]);
                    updateMaterialBase(MaterialBaseId{selectedMaterialBase}, mm);
                }
                ImGui::PopID();

                ImGui::EndTable();
            }
            ImGui::PopID();
        }
        ImGui::PopID();
    }

    void MaterialManager::materialEditor()
    {
        ImGui::PushID("MaterialEditor");
        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit;
        static uint32_t selectedMaterial = 0;

        if (ImGui::BeginTable("Materials", 4, flags)) {

            ImGui::TableSetupColumn("ID");
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Material Base Id");
            ImGui::TableSetupColumn("Action");
            ImGui::TableHeadersRow();

            for (uint32_t row = 0; row < materials_.size(); row++) {
                auto mm = materials_[row];

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%d", row);
                ImGui::TableNextColumn();
                if (ImGui::Selectable(
                    mm.name.c_str(),
                    selectedMaterial == row)) {
                    selectedMaterial = row;
                }
                ImGui::TableNextColumn();
                ImGui::Text("%s", getMaterialBase(mm.materialBaseId).name.c_str());
                ImGui::TableNextColumn();
                //ImGui::TableSetColumnIndex(column);
                if (ImGui::Button("Reload")) {
                    reloadMaterial(MaterialId{row});
                }
                ImGui::SameLine();
                ImGui::Button("Reload All");
                ImGui::SameLine();
                ImGui::Button("Save");
            }
            ImGui::EndTable();
        }

        if (selectedMaterial < materials_.size()) {
            auto mm = materials_[selectedMaterial];

            ImGui::PushID(selectedMaterial);
            std::array<float, 4> param1 = {mm.param1.x, mm.param1.y, mm.param1.z, mm.param1.w};
            std::array<float, 4> param2 = {mm.param2.x, mm.param2.y, mm.param2.z, mm.param2.w};
            std::array<float, 4> param3 = {mm.param3.x, mm.param3.y, mm.param3.z, mm.param3.w};
            std::array<float, 4> param4 = {mm.param4.x, mm.param4.y, mm.param4.z, mm.param4.w};

            if (ImGui::BeginTable(
                "SelectedMaterial",
                2,
                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Material Name");
                ImGui::TableNextColumn();
                ImGui::Text("%s", mm.name.c_str());

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Material Base");
                ImGui::TableNextColumn();
                ImGui::Text("%s", materialBases_[mm.materialBaseId.index()].name.c_str());

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Param 1");
                ImGui::TableNextColumn();
                ImGui::PushID(1);
                if (ImGui::DragFloat4("", param1.data())) {
                    mm.param1 = DirectX::XMFLOAT4(param1[0], param1[1], param1[2], param1[3]);
                    updateMaterial(MaterialId{selectedMaterial}, mm);
                }
                ImGui::PopID();

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Param 2");
                ImGui::TableNextColumn();
                ImGui::PushID(2);
                if (ImGui::DragFloat4("", param2.data())) {
                    mm.param2 = DirectX::XMFLOAT4(param2[0], param2[1], param2[2], param2[3]);
                    updateMaterial(MaterialId{selectedMaterial}, mm);
                }
                ImGui::PopID();

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Param 3");
                ImGui::TableNextColumn();
                ImGui::PushID(3);
                if (ImGui::DragFloat4("", param3.data())) {
                    mm.param3 = DirectX::XMFLOAT4(param3[0], param3[1], param3[2], param3[3]);
                    updateMaterial(MaterialId{selectedMaterial}, mm);
                }
                ImGui::PopID();

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Param 4");
                ImGui::TableNextColumn();
                ImGui::PushID(4);
                if (ImGui::DragFloat4("", param4.data())) {
                    mm.param4 = DirectX::XMFLOAT4(param4[0], param4[1], param4[2], param4[3]);
                    updateMaterial(MaterialId{selectedMaterial}, mm);
                }
                ImGui::PopID();

                ImGui::EndTable();
            }

#if 0
            //mm.param1
            //ImGui::Text("%s", mm.name.c_str());
            if (ImGui::DragFloat4("Param1##material", param1.data())) {
                mm.param1 = DirectX::XMFLOAT4(param1[0], param1[1], param1[2], param1[3]);
                updateMaterial(MaterialId{selectedMaterial}, mm);
            }
            if (ImGui::DragFloat4("Param2##material", param2.data())) {
                mm.param2 = DirectX::XMFLOAT4(param2[0], param2[1], param2[2], param2[3]);
                updateMaterial(MaterialId{selectedMaterial}, mm);
            }
            if (ImGui::DragFloat4("Param3##material", param3.data())) {
                mm.param3 = DirectX::XMFLOAT4(param3[0], param3[1], param3[2], param3[3]);
                updateMaterial(MaterialId{selectedMaterial}, mm);
            }
            if (ImGui::DragFloat4("Param4##material", param4.data())) {
                mm.param4 = DirectX::XMFLOAT4(param4[0], param4[1], param4[2], param4[3]);
                updateMaterial(MaterialId{selectedMaterial}, mm);
            }
#endif
            ImGui::PopID();
        }
        ImGui::PopID();
    }
#if 0
    void MaterialManager::createPipelines(vk::PipelineLayout layout)
    {
        for(uint32_t i = 0; i < materialPipelines_.size(); i++) {
            if(!materialPipelines_[i].pipeline)
            {
                switch(materialPipelines_[i].stage) {
                    case RXAssets::Opaque:
                        materialPipelines_[i].pipeline = createPipeline(i, layout, render)
                        break;
                    case RXAssets::Shadow:
                        break;
                }
            }
        }
        for(auto & pl: materialPipelines_){
            if(!pl.pipeline) {
                createPipeline()
            }
        }
    }
#endif
}
