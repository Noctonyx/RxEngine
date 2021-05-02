#include "Materials.h"

#include "AssetException.h"
#include "EngineMain.hpp"
#include "imgui.h"
#include "Loader.h"
#include "RXCore.h"
#include "Modules/Renderer/Renderer.hpp"
#include "sol/state.hpp"
#include "sol/table.hpp"
#include "RxECS.h"

namespace RxEngine
{
    void MaterialsModule::registerModule()
    {
        world_->set<ComponentGui>(world_->getComponentId<Material>(),
            ComponentGui{ .editor = MaterialsModule::materialGui });
    }
    void MaterialsModule::deregisterModule()
    {
        
    }

    void MaterialsModule::startup()
    {
        world_->createSystem("Material:Pipelines")
              .inGroup("Pipeline:PreFrame")
              .withQuery<MaterialPipelineDetails>()
              .without<HasPipeline>()
              .withRelation<UsesVertexShader, VertexShader>()
              .withRelation<UsesFragmentShader, FragmentShader>()
              .withRelation<UsesLayout, PipelineLayout>()
              .withSingleton<RenderPasses>()
              .each<MaterialPipelineDetails,
                    FragmentShader,
                    VertexShader,
                    PipelineLayout,
                    RenderPasses>(&createPipelines);
    }

    void MaterialsModule::shutdown() { }

    vk::ShaderStageFlags getStageFlags(const std::string & stage)
    {
        if (stage == "both") {
            return vk::ShaderStageFlagBits::eFragment |
                vk::ShaderStageFlagBits::eVertex;
        } else if (stage == "vert") {
            return vk::ShaderStageFlagBits::eVertex;
        } else if (stage == "frag") {
            return vk::ShaderStageFlagBits::eFragment;
        } else {
            throw std::runtime_error(
                R"(Invalid value for stage - valid values: "both", "frag", "vert")");
        }

        //return {};
    }

    void loadShaderData(ecs::World * world, RxCore::Device * device, sol::table & shaders)
    {
        //sol::table shaders = lua["data"]["shaders"];

        for (auto & [key, value]: shaders) {
            auto name = key.as<std::string>();
            sol::table data = value;
            auto stage = data.get<std::string>("stage");
            auto spv = data.get<std::string>("shader");

            RxAssets::ShaderData sd;
            RxAssets::Loader::loadShader(sd, spv);

            auto sh = device->createShader(sd.bytes);

            if (stage == "vert") {
                world->newEntity(name.c_str()).set<VertexShader>({
                    .shader = sh, .shaderAssetName = spv
                });
            } else {
                world->newEntity(name.c_str()).set<FragmentShader>({
                    .shader = sh, .shaderAssetName = spv
                });
            }
        }
    }

    void loadLayout(ecs::World * world,
                    RxCore::Device * device,
                    const std::string & name,
                    sol::table & layout)
    {
        //Render::PipelineLayoutDetails pld;

        vk::PipelineLayoutCreateInfo plci{};
        std::vector<vk::DescriptorSetLayout> dsls{};
        std::vector<vk::PushConstantRange> pcr{};

        PipelineLayout pll;

        sol::table dsLayouts = layout.get<sol::table>("ds_layouts");
        for (auto & [dsLayoutKey, dsLayoutValue]: dsLayouts) {
            sol::table dsLayoutData = dsLayoutValue;

            std::vector<vk::DescriptorSetLayoutBinding> binding = {};
            std::vector<vk::DescriptorBindingFlags> binding_flags = {};

            sol::table bindings = dsLayoutData.get<sol::table>("bindings");
            for (auto & [bindingKey, bindingValue]: bindings) {
                sol::table bindingData = bindingValue;
                auto & b = binding.emplace_back();
                auto & bf = binding_flags.emplace_back();


                b.binding = bindingData.get<uint32_t>("binding");
                b.descriptorCount = bindingData.get_or<uint32_t>("count", 1);

                std::string stage = bindingData.get_or("stage", std::string{"both"});
                std::string type = bindingData.get_or("type", std::string{"combined-sampler"});
                b.stageFlags = getStageFlags(stage);

                if (type == "combined-sampler") {
                    b.descriptorType = vk::DescriptorType::eCombinedImageSampler;
                } else if (type == "storage-buffer") {
                    b.descriptorType = vk::DescriptorType::eStorageBuffer;
                } else if (type == "uniform-buffer") {
                    b.descriptorType = vk::DescriptorType::eUniformBuffer;
                } else if (type == "uniform-buffer-dynamic") {
                    b.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
                } else if (type == "storage-buffer-dynamic") {
                    b.descriptorType = vk::DescriptorType::eStorageBufferDynamic;
                } else {
                    throw std::runtime_error(
                        R"(Invalid value for stage - valid values: "combined-sampler", "storage-buffer", "uniform-buffer", "storage-buffer-dynamic", "uniform-buffer-dynamic")");
                }

                if (bindingData.get_or("variable", false)) {
                    bf |= vk::DescriptorBindingFlagBits::eVariableDescriptorCount;
                }
                if (bindingData.get_or("partially_bound", false)) {
                    bf |= vk::DescriptorBindingFlagBits::ePartiallyBound;
                }
                if (bindingData.get_or("update_after", false)) {
                    bf |= vk::DescriptorBindingFlagBits::eUpdateAfterBind;
                }
            }

            vk::DescriptorSetLayoutBindingFlagsCreateInfo dslbfc{};
            dslbfc.setBindingFlags(binding_flags);

            vk::DescriptorSetLayoutCreateInfo dslci{};
            dslci.setFlags(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool);

            dslci.setPNext(&dslbfc);
            dslci.setBindings(binding);

            auto dsl = device->createDescriptorSetLayout(dslci);

            pll.dsls.push_back(dsl);
            dsls.push_back(dsl);
        }

        sol::table pushConstants = layout.get<sol::table>("push_constants");
        for (auto & [pcKey, pcValue]: pushConstants) {
            sol::table pcData = pcValue;

            auto & p = pcr.emplace_back();
            std::string stage = pcData.get_or("stage", std::string{"both"});
            p.stageFlags = getStageFlags(stage);
            p.offset = pcData.get<uint32_t>("offset");
            p.size = pcData.get<uint32_t>("size");
        }

        plci.setSetLayouts(dsls)
            .setPushConstantRanges(pcr);

        auto l = device->createPipelineLayout(plci);

        pll.layout = l;
        world->newEntity(name.c_str())
             .set<PipelineLayout>(pll);
    }

    void loadLayouts(ecs::World * world, RxCore::Device * device, sol::table & layouts)
    {
        for (auto & [key, value]: layouts) {
            auto layoutName = key.as<std::string>();

            sol::table details = value;

            loadLayout(world, device, layoutName, details);
        }
    }

    RxAssets::MaterialPipelineBlendFactor getBlendFactor(const std::string & value)
    {
        if (value == "zero") {
            return RxAssets::MaterialPipelineBlendFactor::eZero;
        }
        if (value == "one") {
            return RxAssets::MaterialPipelineBlendFactor::eOne;
        }
        if (value == "src-color") {
            return RxAssets::MaterialPipelineBlendFactor::eSrcColor;
        }
        if (value == "1-src-color") {
            return RxAssets::MaterialPipelineBlendFactor::eOneMinusSrcColor;
        }
        if (value == "dest-color") {
            return RxAssets::MaterialPipelineBlendFactor::eDstColor;
        }
        if (value == "src-alpha") {
            return RxAssets::MaterialPipelineBlendFactor::eSrcAlpha;
        }
        if (value == "1-src-alpha") {
            return RxAssets::MaterialPipelineBlendFactor::eOneMinusSrcAlpha;
        }
        if (value == "dest-alpha") {
            return RxAssets::MaterialPipelineBlendFactor::eDstAlpha;
        }
        throw std::runtime_error(
            R"(Invalid value for blendFactor - valid values: "zero", "one", "src-color", "1-src-color", "dest-color", "src-alpha", "1-src-alpha", "dest-alpha")");
    }

    RxAssets::MaterialPipelineBlendOp getBlendOp(const std::string & value)
    {
        if (value == "add") {
            return RxAssets::MaterialPipelineBlendOp::eAdd;
        }
        if (value == "subtract") {
            return RxAssets::MaterialPipelineBlendOp::eSubtract;
        }
        if (value == "reverse-subtract") {
            return RxAssets::MaterialPipelineBlendOp::eReverseSubtract;
        }
        if (value == "min") {
            return RxAssets::MaterialPipelineBlendOp::eMin;
        }
        if (value == "max") {
            return RxAssets::MaterialPipelineBlendOp::eMax;
        }
        throw std::runtime_error(
            R"(Invalid value for blendOp - valid values: "add", "subtract", "reverse-subtract", "min", "max")");
    }

    void populateMaterialPipelineDetails(sol::table & details,
                                         MaterialPipelineDetails & mpd)
    {
        mpd.depthTestEnable = details.get_or("depthTestEnable", false);
        mpd.depthWriteEnable = details.get_or("depthWriteEnable", false);
        mpd.stencilTest = details.get_or("stencilTest", false);
        mpd.depthClamp = details.get_or("depthClamp", false);

        mpd.lineWidth = details.get_or("lineWidth", 1.0f);
        mpd.minDepth = details.get_or("minDepth", 0.0f);
        mpd.maxDepth = details.get_or("maxDepth", 1.0f);

        std::string stage = details.get_or("renderStage", std::string{"opaque"});
        if (stage == "opaque") {
            mpd.stage = RxAssets::PipelineRenderStage::Opaque;
        } else if (stage == "shadow") {
            mpd.stage = RxAssets::PipelineRenderStage::Shadow;
        } else if (stage == "transparent") {
            mpd.stage = RxAssets::PipelineRenderStage::Transparent;
        } else if (stage == "ui") {
            mpd.stage = RxAssets::PipelineRenderStage::UI;
        } else {
            throw std::runtime_error(
                R"(Invalid value for renderStage - valid values: "opaque", "shadow", "transparent", "ui")");
        }

        std::string fillMode = details.get_or("fillMode", std::string{"fill"});
        if (fillMode == "fill") {
            mpd.fillMode = RxAssets::MaterialPipelineFillMode::eFill;
        } else if (fillMode == "line") {
            mpd.fillMode = RxAssets::MaterialPipelineFillMode::eLine;
        } else if (fillMode == "point") {
            mpd.fillMode = RxAssets::MaterialPipelineFillMode::ePoint;
        } else {
            throw std::runtime_error(
                R"(Invalid value for fillMode - valid values: "fill", "line", "point")");
        }

        std::string cullMode = details.get_or("cullMode", std::string{"back"});

        if (cullMode == "back") {
            mpd.cullMode = RxAssets::MaterialPipelineCullMode::eBack;
        } else if (cullMode == "front") {
            mpd.cullMode = RxAssets::MaterialPipelineCullMode::eFront;
        } else if (cullMode == "both") {
            mpd.cullMode = RxAssets::MaterialPipelineCullMode::eFrontAndBack;
        } else if (cullMode == "none") {
            mpd.cullMode = RxAssets::MaterialPipelineCullMode::eNone;
        } else {
            throw std::runtime_error(
                R"(Invalid value for fillMode - valid values: "back", "front", "both", "none")");
        }

        std::string frontFace = details.get_or("frontFace", std::string{"counter-clockwise"});
        if (frontFace == "counter-clockwise") {
            mpd.frontFace = RxAssets::MaterialPipelineFrontFace::eCounterClockwise;
        } else if (frontFace == "clockwise") {
            mpd.frontFace = RxAssets::MaterialPipelineFrontFace::eClockwise;
        } else {
            throw std::runtime_error(
                R"(Invalid value for frontFace - valid values: "counter-clockwise", "clockwise")");
        }

        std::string depthCompare = details.get_or("depthCompare", std::string{"less-equal"});
        if (depthCompare == "never") {
            mpd.depthCompareOp = RxAssets::MaterialPipelineDepthCompareOp::eNever;
        } else if (depthCompare == "less") {
            mpd.depthCompareOp = RxAssets::MaterialPipelineDepthCompareOp::eLess;
        } else if (depthCompare == "equal") {
            mpd.depthCompareOp = RxAssets::MaterialPipelineDepthCompareOp::eEqual;
        } else if (depthCompare == "less-equal") {
            mpd.depthCompareOp = RxAssets::MaterialPipelineDepthCompareOp::eLessOrEqual;
        } else if (depthCompare == "greater") {
            mpd.depthCompareOp = RxAssets::MaterialPipelineDepthCompareOp::eGreater;
        } else if (depthCompare == "not-equal") {
            mpd.depthCompareOp = RxAssets::MaterialPipelineDepthCompareOp::eNotEqual;
        } else if (depthCompare == "greater-equal") {
            mpd.depthCompareOp = RxAssets::MaterialPipelineDepthCompareOp::eGreaterOrEqual;
        } else if (depthCompare == "always") {
            mpd.depthCompareOp = RxAssets::MaterialPipelineDepthCompareOp::eAlways;
        } else {
            throw std::runtime_error(
                R"(Invalid value for depthCompare - valid values: "never", "less", "equal", "less-equal", "greater", "not-equal", "greater-equal", "always")");
        }

        sol::table vertices = details.get<sol::table>("vertices");
        for (auto & [key, value]: vertices) {
            uint32_t count;
            uint32_t offset;
            sol::table v = value;

            count = v.get<uint32_t>("count");
            offset = v.get<uint32_t>("offset");

            auto & ip = mpd.inputs.emplace_back();
            ip.offset = offset;
            ip.count = count;

            if (v.get<std::string>("type") == std::string{"float"}) {
                ip.inputType = RxAssets::MaterialPipelineInputType::eFloat;
            } else if (v.get<std::string>("type") == std::string{"byte"}) {
                ip.inputType = RxAssets::MaterialPipelineInputType::eByte;
            } else {
                throw std::runtime_error(
                    R"(Invalid value for input type - valid values: "byte", "float")");
            }
        }

        sol::table blends = details.get<sol::table>("blends");
        for (auto & [blendKey, blendValue]: blends) {
            sol::table blendData = blendValue;

            auto & blend = mpd.blends.emplace_back();
            //b.enable = false;
            blend.sourceFactor = RxAssets::MaterialPipelineBlendFactor::eSrcAlpha;
            blend.destFactor = RxAssets::MaterialPipelineBlendFactor::eOneMinusSrcAlpha;
            blend.colorBlendOp = RxAssets::MaterialPipelineBlendOp::eAdd;
            blend.sourceAlphaFactor = RxAssets::MaterialPipelineBlendFactor::eOneMinusSrcAlpha;
            blend.destAlphaFactor = RxAssets::MaterialPipelineBlendFactor::eZero;
            blend.alphaBlendOp = RxAssets::MaterialPipelineBlendOp::eAdd;

            blend.enable = blendData.get_or("enable", false);

            if (sol::optional<std::string> f = blendData.get<sol::optional<std::string>>(
                    "sourceFactor");
                f != sol::nullopt) {
                blend.sourceFactor = getBlendFactor(f.value());
            }
            if (sol::optional<std::string> f = blendData.get<sol::optional<std::string>>(
                    "destFactor");
                f != sol::nullopt) {
                blend.destFactor = getBlendFactor(f.value());
            }
            if (sol::optional<std::string> f = blendData.get<sol::optional<std::string>>(
                    "sourceAlphaFactor");
                f != sol::nullopt) {
                blend.sourceAlphaFactor = getBlendFactor(f.value());
            }
            if (sol::optional<std::string> f = blendData.get<sol::optional<std::string>>(
                    "destAlphaFactor");
                f != sol::nullopt) {
                blend.destAlphaFactor = getBlendFactor(f.value());
            }
            if (sol::optional<std::string> f = blendData.get<sol::optional<std::string>>("colorOp");
                f != sol::nullopt) {
                blend.colorBlendOp = getBlendOp(f.value());
            }
            if (sol::optional<std::string> f = blendData.get<sol::optional<std::string>>("alphaOp");
                f != sol::nullopt) {
                blend.alphaBlendOp = getBlendOp(f.value());
            }
        }
    }

    void loadPipeline(ecs::World * world,
                      RxCore::Device * device,
                      const std::string & name,
                      sol::table & pipeline)
    {
        const std::string vs_name = pipeline["vertexShader"];
        const std::string fs_name = pipeline["fragmentShader"];
        const std::string layout_name = pipeline["layout"];

        MaterialPipelineDetails mpd;

        populateMaterialPipelineDetails(pipeline, mpd);

        auto vse = world->lookup(vs_name.c_str());
        if (!vse.isAlive() || !vse.has<VertexShader>()) {
            spdlog::critical("Missing shader for pipeline {}", name.c_str());
            throw RxAssets::AssetException("missing shader:", vs_name);
        }

        auto fse = world->lookup(fs_name.c_str());
        if (!fse.isAlive() || !fse.has<FragmentShader>()) {
            spdlog::critical("Missing shader for pipeline {}", name.c_str());
            throw RxAssets::AssetException("missing shader:", fs_name);
        }

        auto lay = world->lookup(layout_name.c_str());
        if (!lay.isAlive() || !lay.has<PipelineLayout>()) {
            spdlog::critical("Missing layout for pipeline {}", name.c_str());
            throw RxAssets::AssetException("missing shader:", layout_name);
        }

        world->newEntity(name.c_str())
             .set<MaterialPipelineDetails>({mpd})
             .set<UsesVertexShader>({{vse.id}})
             .set<UsesFragmentShader>({{fse.id}})
             .set<UsesLayout>({{lay.id}});
    }

    void loadPipelines(ecs::World * world, RxCore::Device * device, sol::table & pipelines)
    {
        for (auto & [key, value]: pipelines) {
            auto pipelineName = key.as<std::string>();

            sol::table details = value;

            loadPipeline(world, device, pipelineName, details);
        }
    }


    void getSamplerDetails(RxAssets::SamplerData & sd, sol::table & sampler)
    {
        std::string minFilter = sampler.get_or("minFilter", std::string{"nearest"});
        if (minFilter == "nearest") {
            sd.minFilter = VK_FILTER_NEAREST;
        } else if (minFilter == "linear") {
            sd.minFilter = VK_FILTER_LINEAR;
        } else {
            throw std::runtime_error(
                R"(Invalid value for minFilter - valid values: "nearest", "linear")");
        }

        std::string magFilter = sampler.get_or("magFilter", std::string{"nearest"});
        if (magFilter == "nearest") {
            sd.magFilter = VK_FILTER_NEAREST;
        } else if (magFilter == "linear") {
            sd.magFilter = VK_FILTER_LINEAR;
        } else {
            throw std::runtime_error(
                R"(Invalid value for magFilter - valid values: "nearest", "linear")");
        }

        std::string mipMapMode = sampler.get_or("mipMapMode", std::string{"nearest"});
        if (mipMapMode == "nearest") {
            sd.mipMapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        } else if (mipMapMode == "linear") {
            sd.mipMapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        } else {
            throw std::runtime_error(
                R"(Invalid value for mipMapMode - valid values: "nearest", "linear")");
        }

        std::string addressU = sampler.get_or("addressU", std::string{"nearest"});
        if (addressU == "repeat") {
            sd.addressU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        } else if (addressU == "mirrored-repeat") {
            sd.addressU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        } else if (addressU == "clamp-edge") {
            sd.addressU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        } else if (addressU == "clamp-border") {
            sd.addressU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        } else {
            throw std::runtime_error(
                R"(Invalid value for addressU - valid values: "repeat", "mirrored-repeat", "clamp-edge", "clamp-border")");
        }

        std::string addressV = sampler.get_or("addressV", std::string{"nearest"});
        if (addressV == "repeat") {
            sd.addressV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        } else if (addressV == "mirrored-repeat") {
            sd.addressV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        } else if (addressV == "clamp-edge") {
            sd.addressV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        } else if (addressV == "clamp-border") {
            sd.addressV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        } else {
            throw std::runtime_error(
                R"(Invalid value for addressV - valid values: "repeat", "mirrored-repeat", "clamp-edge", "clamp-border")");
        }

        std::string addressW = sampler.get_or("addressW", std::string{"nearest"});
        if (addressW == "nearest") {
            sd.addressW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        } else if (addressW == "mirrored-repeat") {
            sd.addressW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        } else if (addressW == "clamp-edge") {
            sd.addressW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        } else if (addressW == "clamp-border") {
            sd.addressW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        } else {
            throw std::runtime_error(
                R"(Invalid value for addressW - valid values: "repeat", "mirrored-repeat", "clamp-edge", "clamp-border")");
        }

        sd.mipLodBias = sampler.get_or("mipLodBias", 0.f);
        sd.anisotropy = sampler.get_or("anisotropy", false);
        sd.maxAnisotropy = sampler.get_or("maxAnisotropy", 0.f);
        sd.minLod = sampler.get_or("minLod", 0.f);
        sd.maxLod = sampler.get_or("maxLod", VK_LOD_CLAMP_NONE);

        std::string borderColor = sampler.get_or("borderColor",
                                                 std::string{"float-transparent-black"});
        if (borderColor == "float-transparent-black") {
            sd.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        } else if (borderColor == "int-transparent-black") {
            sd.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
        } else if (borderColor == "float-opaque-black") {
            sd.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        } else if (borderColor == "int-opaque-black") {
            sd.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        } else if (borderColor == "float-opaque-white") {
            sd.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        } else if (borderColor == "int-opaque-white") {
            sd.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
        } else {
            throw std::runtime_error(
                R"(Invalid value for borderColor - valid values are "float-transparent-black", "int-transparent-black", "float-opaque-black", "int-opaque-black", "float-opaque-white", "int-opaque-white" )");
        }
    }

    vk::Sampler createSampler(RxCore::Device * device, RxAssets::SamplerData & sd)
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

        return device->createSampler(sci);
    }

    ecs::EntityHandle loadOrGetImage(const std::string & name,
                                     ecs::World * world,
                                     RxCore::Device * device)
    {
        auto e = world->lookup(name.c_str());

        if (e.isAlive() && e.has<MaterialImage>()) {
            return e;
        }

        RxAssets::ImageData id{};
        RxAssets::Loader::loadImage(id, name);

        auto image = device->createImage(
            id.imType == RxAssets::eBC7
                ? vk::Format::eBc7UnormBlock
                : vk::Format::eR8G8B8A8Unorm,
            vk::Extent3D{id.width, id.height, 1},
            static_cast<uint32_t>(id.mipLevels.size()),
            1,
            vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
            vk::ImageType::e2D);

        auto iv =
            image->createImageView(vk::ImageViewType::e2D, vk::ImageAspectFlagBits::eColor);

        for (uint32_t j = 0; j < id.mipLevels.size(); j++) {
            const auto staging_buffer = device->createStagingBuffer(
                id.mipLevels[j].bytes.size(), id.mipLevels[j].bytes.data());

            device->transferBufferToImage(
                staging_buffer,
                image,
                vk::Extent3D(id.mipLevels[j].width, id.mipLevels[j].height, 1),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                1,
                0,
                j);
        }
        MaterialImage mi;
        mi.image = image;
        mi.imageView = iv;

        return world->newEntity(name.c_str()).set<MaterialImage>(mi);
    }

    void loadTexture(ecs::World * world,
                     RxCore::Device * device,
                     std::string textureName,
                     sol::table details)
    {
        const std::string file_name = details["image"];

        const auto image_entity = loadOrGetImage(file_name, world, device);

        RxAssets::SamplerData sd{};

        sol::table sampler = details.get<sol::table>("sampler");
        getSamplerDetails(sd, sampler);

        const auto sampler_handle = createSampler(device, sd);

        world->newEntity(textureName.c_str())
             .set<ecs::InstanceOf>({{image_entity.id}})
             .set<Render::MaterialSampler>({sampler_handle, 9999});
    }

    void loadTextures(ecs::World * world, RxCore::Device * device, sol::table & textures)
    {
        for (auto & [key, value]: textures) {
            const std::string texture_name = key.as<std::string>();
            sol::table details = value;

            loadTexture(world, device, texture_name, details);
        }
    }

    void loadMaterial(ecs::World * world,
                      RxCore::Device * device,
                      const std::string & name,
                      sol::table & material)
    {
        sol::optional<std::string> opaquePipeline = material.get<sol::optional<std::string>>(
            "opaquePipeline");
        sol::optional<std::string> shadowPipeline = material.get<sol::optional<std::string>>(
            "shadowPipeline");
        sol::optional<std::string> transparentPipeline = material.get<sol::optional<std::string>>(
            "transparentPipeline");
        sol::optional<std::string> uiPipeline = material.get<sol::optional<std::string>>(
            "uiPipeline");

        Material mi{};

        auto e = world->lookup(name.c_str());

        if (!e.isAlive()) {
            e = world->newEntity(name.c_str());
        }

        if (opaquePipeline.has_value()) {
            auto eop = world->lookup(opaquePipeline.value().c_str());

            auto mpd = eop.get<MaterialPipelineDetails>();
            if (!mpd || mpd->stage != RxAssets::PipelineRenderStage::Opaque) {
                throw RxAssets::AssetException("Not a valid opaquePipeline:", name);
            }
            e.set<HasOpaquePipeline>({{eop.id}});
        }

        if (shadowPipeline.has_value()) {
            auto eop = world->lookup(shadowPipeline.value().c_str());

            auto mpd = eop.get<MaterialPipelineDetails>();
            if (!mpd || mpd->stage != RxAssets::PipelineRenderStage::Shadow) {
                throw RxAssets::AssetException("Not a valid shadowPipeline:", name);
            }
            e.set<HasShadowPipeline>({{eop.id}});
        }

        if (transparentPipeline.has_value()) {
            auto eop = world->lookup(transparentPipeline.value().c_str());

            auto mpd = eop.get<MaterialPipelineDetails>();
            if (!mpd || mpd->stage != RxAssets::PipelineRenderStage::Transparent) {
                throw RxAssets::AssetException("Not a valid transparentPipeline:", name);
            }
            e.set<HasTransparentPipeline>({{eop.id}});
        }

        if (uiPipeline.has_value()) {
            auto eop = world->lookup(uiPipeline.value().c_str());

            auto mpd = eop.get<MaterialPipelineDetails>();
            if (!mpd || mpd->stage != RxAssets::PipelineRenderStage::UI) {
                throw RxAssets::AssetException("Not a valid uiPipeline:", name);
            }
            e.set<HasUiPipeline>({{eop.id}});
        }

        float roughness = material.get_or("roughness", 0.f);
        float metallic = material.get_or("metallic", 0.f);

        mi.roughness = roughness;
        mi.metallic = metallic;

        sol::optional<std::string> colorTexture = material.get<sol::optional<std::string>>(
            "color_texture");

        if (colorTexture.has_value()) {
            auto te = world->lookup(colorTexture.value().c_str());
            if (te.isAlive() && te.has<Render::MaterialSampler>()) {
                mi.materialTextures[0] = te.id;
            } else {
                throw RxAssets::AssetException("Not a valid texture:", name);
            }
        }

        std::string a = material.get_or("alpha_mode", std::string{"OPAQUE"});
        if (a == "OPAQUE") {
            mi.alpha = MaterialAlphaMode::Opaque;
        } else {
            mi.alpha = MaterialAlphaMode::Transparent;
        }

        e.set<Material>(mi);
    }

    void loadMaterials(ecs::World * world, RxCore::Device * device, sol::table & materials)
    {
        for (auto & [key, value]: materials) {
            const std::string name = key.as<std::string>();
            sol::table details = value;
            loadMaterial(world, device, name, details);
        }
    }

    void MaterialsModule::processStartupData(sol::state * lua, RxCore::Device * device)
    {
        sol::table data = lua->get<sol::table>("data");

        sol::table shaders = data.get<sol::table>("shaders");
        sol::table layouts = data.get<sol::table>("pipeline_layouts");
        sol::table textures = data.get<sol::table>("textures");
        sol::table pipelines = data.get<sol::table>("material_pipelines");
        sol::table materials = data.get<sol::table>("materials");

        loadShaderData(world_, device, shaders);
        loadLayouts(world_, device, layouts);
        loadPipelines(world_, device, pipelines);
        loadTextures(world_, device, textures);
        loadMaterials(world_, device, materials);
    }

    vk::Pipeline MaterialsModule::createMaterialPipeline(const MaterialPipelineDetails * mpd,
                                                         const FragmentShader * frag,
                                                         const VertexShader * vert,
                                                         vk::PipelineLayout layout,
                                                         vk::RenderPass rp,
                                                         uint32_t subpass)
    {
        vk::GraphicsPipelineCreateInfo gpci{};
        vk::PipelineDynamicStateCreateInfo pdsci{};
        vk::PipelineColorBlendStateCreateInfo pcbsci{};
        vk::PipelineDepthStencilStateCreateInfo pdssci{};
        vk::PipelineMultisampleStateCreateInfo pmsci{};
        vk::PipelineRasterizationStateCreateInfo prsci{};
        vk::PipelineViewportStateCreateInfo pvsci{};
        vk::PipelineInputAssemblyStateCreateInfo piasci{};
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

        prsci.setLineWidth(mpd->lineWidth)
             .setPolygonMode(static_cast<vk::PolygonMode>(mpd->fillMode))
             .setDepthClampEnable(mpd->depthClamp)
             .setRasterizerDiscardEnable(false)
             .setCullMode(static_cast<vk::CullModeFlagBits>(mpd->cullMode))
             .setFrontFace(static_cast<vk::FrontFace>(mpd->frontFace));

        pmsci.setRasterizationSamples(vk::SampleCountFlagBits::e1);

        shaderStages.push_back(
            vk::PipelineShaderStageCreateInfo{
                {},
                vk::ShaderStageFlagBits::eVertex,
                vert->shader->Handle(),
                "main"
            }
        );
        shaderStages.push_back(
            vk::PipelineShaderStageCreateInfo{
                {},
                vk::ShaderStageFlagBits::eFragment,
                frag->shader->Handle(),
                "main"
            }
        );
#if 0
        vk::PipelineShaderStageCreateInfo& st = shaderStages.emplace_back();
        st.setStage(vk::ShaderStageFlagBits::eVertex).setModule(mp.vertexShader);
        st.setPName("main");
        st = shaderStages.emplace_back();
        st.setStage(vk::ShaderStageFlagBits::eFragment).setModule(mp.fragmentShader);
        st.setPName("main");
#endif
        for (auto & mpa: mpd->blends) {
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

        pdssci.setDepthTestEnable(mpd->depthWriteEnable)
              .setDepthWriteEnable(mpd->depthTestEnable)
              .setDepthCompareOp(static_cast<vk::CompareOp>(mpd->depthCompareOp))
              .setDepthBoundsTestEnable(false)
              .setStencilTestEnable(mpd->stencilTest)
              .setFront({vk::StencilOp::eKeep, vk::StencilOp::eKeep})
              .setBack({vk::StencilOp::eKeep, vk::StencilOp::eKeep})
              .setMinDepthBounds(mpd->minDepth)
              .setMaxDepthBounds(mpd->maxDepth);

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
            .setRenderPass(rp)
            .setSubpass(subpass);

        std::vector<vk::VertexInputBindingDescription> bindings;
        std::vector<vk::VertexInputAttributeDescription> attributes;

        if (mpd->inputs.size() > 0) {
            uint32_t offset = 0;
            uint32_t loc = 0;

            for (auto & i: mpd->inputs) {
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
                    default: ;
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
                    default: ;
                    }
                }
            }

            bindings.emplace_back(0, offset, vk::VertexInputRate::eVertex);
            pvisci.setVertexBindingDescriptions(bindings).
                   setVertexAttributeDescriptions(attributes);

        }

        pcbsci.setAttachments(attachments);
        pdsci.setDynamicStates(dynamicStates);

        auto rv = RxCore::iVulkan()->getDevice().createGraphicsPipeline(nullptr, gpci);
        assert(rv.result == vk::Result::eSuccess);

        return rv.value;
    }

    void MaterialsModule::createPipelines(
        ecs::EntityHandle e,
        const MaterialPipelineDetails * mpd,
        const FragmentShader * frag,
        const VertexShader * vert,
        const PipelineLayout * pll,
        const RenderPasses * rp)
    {
        if (!rp) {
            return;
        }
        if (vert && frag && mpd) {
            if (mpd->stage == RxAssets::PipelineRenderStage::UI) {
                auto pl = createMaterialPipeline(
                    mpd, frag, vert, pll->layout, rp->uiRenderPass, rp->uiSubPass);
                e.setDeferred<UiPipeline>({
                    std::make_shared<RxCore::Pipeline>(pl), rp->uiRenderPass,
                    rp->uiSubPass
                });
                e.addDeferred<HasPipeline>();
            }

            if (mpd->stage == RxAssets::PipelineRenderStage::Opaque) {
                auto pl = createMaterialPipeline(
                    mpd, frag, vert, pll->layout, rp->opaqueRenderPass,
                    rp->opaqueSubPass);
                e.setDeferred<OpaquePipeline>({
                    std::make_shared<RxCore::Pipeline>(pl), rp->opaqueRenderPass,
                    rp->opaqueSubPass
                });
                e.addDeferred<HasPipeline>();
            }

            if (mpd->stage == RxAssets::PipelineRenderStage::Shadow) {
                auto pl = createMaterialPipeline(
                    mpd, frag, vert, pll->layout, rp->shadowRenderPass,
                    rp->shadowSubPass);
                e.setDeferred<ShadowPipeline>({
                    std::make_shared<RxCore::Pipeline>(pl), rp->shadowRenderPass,
                    rp->shadowSubPass
                });
                e.addDeferred<HasPipeline>();
            }

            if (mpd->stage == RxAssets::PipelineRenderStage::Transparent) {
                auto pl = createMaterialPipeline(
                    mpd, frag, vert, pll->layout, rp->transparentRenderPass,
                    rp->transparentSubPass);
                e.setDeferred<TransparentPipeline>({
                    std::make_shared<RxCore::Pipeline>(pl), rp->transparentRenderPass,
                    rp->transparentSubPass
                });
                e.addDeferred<HasPipeline>();
            }
        }
    }

    void MaterialsModule::materialGui(ecs::EntityHandle e)
    {
        auto material = e.get<Material>();

        if (material) {
            if (ImGui::BeginTable("ComponentGui", 2, /*ImGuiTableFlags_Borders | */
                ImGuiTableFlags_Resizable |
                ImGuiTableFlags_Hideable)) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Value");
                //ImGui::TableHeadersRow();

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Roughness");
                ImGui::TableNextColumn();
                ImGui::Text("%.3f", material->roughness);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Metallic");
                ImGui::TableNextColumn();
                ImGui::Text("%.3f", material->metallic);

                ImGui::EndTable();
            }
        }
    }
}