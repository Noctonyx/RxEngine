////////////////////////////////////////////////////////////////////////////////
// MIT License
//
// Copyright (c) 2021.  Shane Hyde
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

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
#include "Modules/Render.h"

namespace RxEngine
{
    void materialUi(ecs::World *, void * ptr)
    {
        auto material = static_cast<Material *>(ptr);

        if (material) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Tex1");
            ImGui::TableNextColumn();
            ImGui::Text("%lld", material->materialTextures[0]);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Tex1");
            ImGui::TableNextColumn();
            ImGui::Text("%lld", material->materialTextures[1]);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Tex1");
            ImGui::TableNextColumn();
            ImGui::Text("%lld", material->materialTextures[2]);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Tex1");
            ImGui::TableNextColumn();
            ImGui::Text("%lld", material->materialTextures[3]);
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
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Sequence");
            ImGui::TableNextColumn();
            ImGui::Text("%d", material->sequence);
        }
    }

    void MaterialsModule::startup()
    {
        world_->set<ComponentGui>(
            world_->getComponentId<Material>(),
            ComponentGui{.editor = materialUi}
        );

        world_->createSystem("Material:Pipelines")
              .inGroup("Pipeline:PreFrame")
              .withQuery<MaterialPipelineDetails>()
              .without<GraphicsPipeline>()
              .withRelation<UsesVertexShader, VertexShader>()
              .withRelation<UsesFragmentShader, FragmentShader>()
              .withRelation<UsesLayout, PipelineLayout>()
              .withSingleton<RenderPasses>()
              .each<MaterialPipelineDetails,
                    FragmentShader,
                    VertexShader,
                    PipelineLayout,
                    RenderPasses>(
                  [this](ecs::EntityHandle e,
                         const MaterialPipelineDetails * mpd,
                         const FragmentShader * frag,
                         const VertexShader * vert,
                         const PipelineLayout * pll,
                         const RenderPasses * rp)
                  {
                      createPipelines(e, mpd, frag, vert, pll, rp);
                  }
              );

        world_->createSystem("Material:setDescriptor")
              .inGroup("Pipeline:PreFrame")
              .withQuery<DescriptorSet>()
              .without<MaterialDescriptor>()
              .withRead<Material>()
              .withRead<MaterialImage>()
              .each<DescriptorSet>(
                  [this](ecs::EntityHandle e, DescriptorSet * ds)
                  {
                      createShaderMaterialData(e, ds);
                  }
              );

        materialQuery = world_->createQuery<Material>().id;
    }

    void MaterialsModule::shutdown()
    {
        world_->remove<ComponentGui>(world_->getComponentId<Material>());
        world_->lookup("Material:Pipelines").destroy();
        world_->lookup("Material:setDescriptor").destroy();
    }

    VkShaderStageFlags getStageFlags(const std::string & stage)
    {
        if (stage == "both") {
            return VK_SHADER_STAGE_FRAGMENT_BIT |
                VK_SHADER_STAGE_VERTEX_BIT;
        } else if (stage == "vert") {
            return VK_SHADER_STAGE_VERTEX_BIT;
        } else if (stage == "frag") {
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        } else {
            throw std::runtime_error(
                R"(Invalid value for stage - valid values: "both", "frag", "vert")"
            );
        }

        //return {};
    }

    void loadShaderData(ecs::World * world, RxCore::Device * device, sol::table & shaders)
    {
        //sol::table shaders = lua["data"]["shaders"];

        for (auto & [key, value]: shaders) {
            auto name = key.as<std::string>();
            spdlog::debug("Loading shader {0} to world", name);
            sol::table data = value;
            auto stage = data.get<std::string>("stage");
            auto spv = data.get<std::string>("shader");

            RxAssets::ShaderData sd;
            RxAssets::Loader::loadShader(sd, spv);

            auto sh = device->createShader(sd.bytes);

            if (stage == "vert") {
                world->newEntityReplace(name.c_str()).set<VertexShader>(
                    {
                        .shader = sh, .shaderAssetName = spv
                    }
                );
            } else {
                world->newEntityReplace(name.c_str()).set<FragmentShader>(
                    {
                        .shader = sh, .shaderAssetName = spv
                    }
                );
            }
        }
    }

    void loadLayout(ecs::World * world,
                    RxCore::Device * device,
                    const std::string & name,
                    sol::table & layout)
    {
        //Render::PipelineLayoutDetails pld;
        spdlog::debug("Loading layout {0} to world", name);

        VkPipelineLayoutCreateInfo plci{};
        plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        std::vector<VkDescriptorSetLayout> dsls{};
        std::vector<VkPushConstantRange> pcr{};

        PipelineLayout pll{};

        sol::table dsLayouts = layout.get<sol::table>("ds_layouts");
        for (auto & [dsLayoutKey, dsLayoutValue]: dsLayouts) {
            sol::table dsLayoutData = dsLayoutValue;
            pll.counts.clear();
            std::vector<VkDescriptorSetLayoutBinding> binding = {};
            std::vector<VkDescriptorBindingFlags> binding_flags = {};

            sol::table bindings = dsLayoutData.get<sol::table>("bindings");
            for (auto & [bindingKey, bindingValue]: bindings) {
                sol::table bindingData = bindingValue;
                auto & b = binding.emplace_back();
                auto & bf = binding_flags.emplace_back();

                b.binding = bindingData.get<uint32_t>("binding");
                b.descriptorCount = bindingData.get_or<uint32_t>("count", 1);

                if (b.descriptorCount != 1) {
                    pll.counts.push_back(b.descriptorCount);
                }

                std::string stage = bindingData.get_or("stage", std::string{"both"});
                std::string type = bindingData.get_or("type", std::string{"combined-sampler"});
                b.stageFlags = getStageFlags(stage);

                if (type == "combined-sampler") {
                    b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                } else if (type == "storage-buffer") {
                    b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                } else if (type == "uniform-buffer") {
                    b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                } else if (type == "uniform-buffer-dynamic") {
                    b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                } else if (type == "storage-buffer-dynamic") {
                    b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
                } else {
                    throw std::runtime_error(
                        R"(Invalid value for stage - valid values: "combined-sampler", "storage-buffer", "uniform-buffer", "storage-buffer-dynamic", "uniform-buffer-dynamic")"
                    );
                }

                if (bindingData.get_or("variable", false)) {
                    bf |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
                }
                if (bindingData.get_or("partially_bound", false)) {
                    bf |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
                }
                if (bindingData.get_or("update_after", false)) {
                    bf |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
                }
            }

            VkDescriptorSetLayoutBindingFlagsCreateInfo dslbfc{};
            dslbfc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
            dslbfc.bindingCount = static_cast<uint32_t>(binding_flags.size());
            dslbfc.pBindingFlags = binding_flags.data();

            VkDescriptorSetLayoutCreateInfo dslci{};
            dslci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            dslci.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
            dslci.pNext = &dslbfc;
            dslci.bindingCount = static_cast<uint32_t>(binding.size());
            dslci.pBindings = binding.data();

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
        plci.setLayoutCount = static_cast<uint32_t>(dsls.size());
        plci.pSetLayouts = dsls.data();
        plci.pushConstantRangeCount = static_cast<uint32_t>(pcr.size());
        plci.pPushConstantRanges = pcr.data();

        auto l = device->createPipelineLayout(plci);

        pll.layout = l;
        world->newEntityReplace(name.c_str())
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
            R"(Invalid value for blendFactor - valid values: "zero", "one", "src-color", "1-src-color", "dest-color", "src-alpha", "1-src-alpha", "dest-alpha")"
        );
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
            R"(Invalid value for blendOp - valid values: "add", "subtract", "reverse-subtract", "min", "max")"
        );
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
                R"(Invalid value for renderStage - valid values: "opaque", "shadow", "transparent", "ui")"
            );
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
                R"(Invalid value for fillMode - valid values: "fill", "line", "point")"
            );
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
                R"(Invalid value for fillMode - valid values: "back", "front", "both", "none")"
            );
        }

        std::string frontFace = details.get_or("frontFace", std::string{"counter-clockwise"});
        if (frontFace == "counter-clockwise") {
            mpd.frontFace = RxAssets::MaterialPipelineFrontFace::eCounterClockwise;
        } else if (frontFace == "clockwise") {
            mpd.frontFace = RxAssets::MaterialPipelineFrontFace::eClockwise;
        } else {
            throw std::runtime_error(
                R"(Invalid value for frontFace - valid values: "counter-clockwise", "clockwise")"
            );
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
                R"(Invalid value for depthCompare - valid values: "never", "less", "equal", "less-equal", "greater", "not-equal", "greater-equal", "always")"
            );
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
                    R"(Invalid value for input type - valid values: "byte", "float")"
                );
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
                    "sourceFactor"
                );
                f != sol::nullopt) {
                blend.sourceFactor = getBlendFactor(f.value());
            }
            if (sol::optional<std::string> f = blendData.get<sol::optional<std::string>>(
                    "destFactor"
                );
                f != sol::nullopt) {
                blend.destFactor = getBlendFactor(f.value());
            }
            if (sol::optional<std::string> f = blendData.get<sol::optional<std::string>>(
                    "sourceAlphaFactor"
                );
                f != sol::nullopt) {
                blend.sourceAlphaFactor = getBlendFactor(f.value());
            }
            if (sol::optional<std::string> f = blendData.get<sol::optional<std::string>>(
                    "destAlphaFactor"
                );
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

        spdlog::debug("Loading pipeline {0} to world", name);

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

        world->newEntityReplace(name.c_str())
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
                R"(Invalid value for minFilter - valid values: "nearest", "linear")"
            );
        }

        std::string magFilter = sampler.get_or("magFilter", std::string{"nearest"});
        if (magFilter == "nearest") {
            sd.magFilter = VK_FILTER_NEAREST;
        } else if (magFilter == "linear") {
            sd.magFilter = VK_FILTER_LINEAR;
        } else {
            throw std::runtime_error(
                R"(Invalid value for magFilter - valid values: "nearest", "linear")"
            );
        }

        std::string mipMapMode = sampler.get_or("mipMapMode", std::string{"nearest"});
        if (mipMapMode == "nearest") {
            sd.mipMapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        } else if (mipMapMode == "linear") {
            sd.mipMapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        } else {
            throw std::runtime_error(
                R"(Invalid value for mipMapMode - valid values: "nearest", "linear")"
            );
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
                R"(Invalid value for addressU - valid values: "repeat", "mirrored-repeat", "clamp-edge", "clamp-border")"
            );
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
                R"(Invalid value for addressV - valid values: "repeat", "mirrored-repeat", "clamp-edge", "clamp-border")"
            );
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
                R"(Invalid value for addressW - valid values: "repeat", "mirrored-repeat", "clamp-edge", "clamp-border")"
            );
        }

        sd.mipLodBias = sampler.get_or("mipLodBias", 0.f);
        sd.anisotropy = sampler.get_or("anisotropy", false);
        sd.maxAnisotropy = sampler.get_or("maxAnisotropy", 0.f);
        sd.minLod = sampler.get_or("minLod", 0.f);
        sd.maxLod = sampler.get_or("maxLod", VK_LOD_CLAMP_NONE);

        std::string borderColor = sampler.get_or(
            "borderColor",
            std::string{"float-transparent-black"}
        );
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
                R"(Invalid value for borderColor - valid values are "float-transparent-black", "int-transparent-black", "float-opaque-black", "int-opaque-black", "float-opaque-white", "int-opaque-white" )"
            );
        }
    }

    VkSampler createSampler(RxCore::Device * device, RxAssets::SamplerData & sd)
    {
        VkSamplerCreateInfo sci{};
        sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        sci.minFilter = (static_cast<VkFilter>(sd.minFilter));
        sci.magFilter = (static_cast<VkFilter>(sd.magFilter));
        sci.mipmapMode = (static_cast<VkSamplerMipmapMode>(sd.mipMapMode));
        sci.addressModeU = (static_cast<VkSamplerAddressMode>(sd.addressU));
        sci.addressModeV = (static_cast<VkSamplerAddressMode>(sd.addressV));
        sci.addressModeW = (static_cast<VkSamplerAddressMode>(sd.addressW));
        sci.mipLodBias = (sd.mipLodBias);
        sci.anisotropyEnable = (sd.anisotropy);
        sci.maxAnisotropy = (sd.maxAnisotropy);
        sci.minLod = (sd.minLod);
        sci.maxLod = (sd.maxLod);

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
                ? VK_FORMAT_BC7_UNORM_BLOCK
                : VK_FORMAT_R8G8B8A8_UNORM,
            VkExtent3D{id.width, id.height, 1},
            static_cast<uint32_t>(id.mipLevels.size()),
            1,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_TYPE_2D
        );

        auto iv =
            device->createImageView(image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT);

        for (uint32_t j = 0; j < id.mipLevels.size(); j++) {
            const auto staging_buffer = device->createStagingBuffer(
                id.mipLevels[j].bytes.size(), id.mipLevels[j].bytes.data());

            device->transferBufferToImage(
                staging_buffer,
                image,
                VkExtent3D{ id.mipLevels[j].width, id.mipLevels[j].height, 1 },
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                1,
                0,
                j
            );
        }
        MaterialImage mi;
        mi.image = image;
        mi.imageView = iv;

        return world->newEntityReplace(name.c_str()).set<MaterialImage>(mi);
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

        world->newEntityReplace(textureName.c_str())
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
            "opaquePipeline"
        );
        sol::optional<std::string> shadowPipeline = material.get<sol::optional<std::string>>(
            "shadowPipeline"
        );
        sol::optional<std::string> transparentPipeline = material.get<sol::optional<std::string>>(
            "transparentPipeline"
        );
        sol::optional<std::string> uiPipeline = material.get<sol::optional<std::string>>(
            "uiPipeline"
        );

        Material mi{};
#if 0
        auto e = world->lookup(name.c_str());

        if (!e.isAlive()) {
            e = world->newEntity(name.c_str());
        }
#endif
        auto e = world->newEntityReplace(name.c_str());

        std::string a = material.get_or("alpha_mode", std::string{"OPAQUE"});
        if (a == "OPAQUE") {
            mi.alpha = MaterialAlphaMode::Opaque;
        } else {
            mi.alpha = MaterialAlphaMode::Transparent;
        }

        if (opaquePipeline.has_value()) {
            auto eop = world->lookup(opaquePipeline.value().c_str());

            auto mpd = eop.get<MaterialPipelineDetails>();
            if (!mpd || mpd->stage != RxAssets::PipelineRenderStage::Opaque) {
                throw RxAssets::AssetException("Not a valid opaquePipeline:", name);
            }
            e.set<HasOpaquePipeline>({{eop.id}});
        } else if (mi.alpha == MaterialAlphaMode::Opaque) {
            auto eop = world->lookup("pipeline/staticmesh_opaque");

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
#if 0
        else if (mi.alpha == MaterialAlphaMode::Opaque) {
            auto eop = world->lookup("pipeline/staticmesh_shadow");

            auto mpd = eop.get<MaterialPipelineDetails>();
            if (!mpd || mpd->stage != RxAssets::PipelineRenderStage::Shadow) {
                throw RxAssets::AssetException("Not a valid opaquePipeline:", name);
            }
            e.set<HasShadpwPipeline>({ {eop.id} });
        }
#endif
        if (transparentPipeline.has_value()) {
            auto eop = world->lookup(transparentPipeline.value().c_str());

            auto mpd = eop.get<MaterialPipelineDetails>();
            if (!mpd || mpd->stage != RxAssets::PipelineRenderStage::Transparent) {
                throw RxAssets::AssetException("Not a valid transparentPipeline:", name);
            }
            e.set<HasTransparentPipeline>({{eop.id}});
        } else if (mi.alpha == MaterialAlphaMode::Transparent) {
            auto eop = world->lookup("pipeline/staticmesh_transparent");

            auto mpd = eop.get<MaterialPipelineDetails>();
            if (!mpd || mpd->stage != RxAssets::PipelineRenderStage::Transparent) {
                throw RxAssets::AssetException("Not a valid opaquePipeline:", name);
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
            "color_texture"
        );

        if (colorTexture.has_value()) {
            auto te = world->lookup(colorTexture.value().c_str());
            if (te.isAlive() && te.has<Render::MaterialSampler>()) {
                mi.materialTextures[0] = te.id;
            } else {
                throw RxAssets::AssetException("Not a valid texture:", name);
            }
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

    void MaterialsModule::loadData(sol::table data)
    {
        sol::optional<sol::table> shaders = data["shader"];
        sol::optional<sol::table> layouts = data["pipeline_layout"];
        sol::optional<sol::table> textures = data["texture"];
        sol::optional<sol::table> pipelines = data["material_pipeline"];
        sol::optional<sol::table> materials = data["material"];

        auto device = engine_->getDevice();

        if (shaders.has_value()) {
            loadShaderData(world_, device, shaders.value());
        }
        if (layouts.has_value()) {
            loadLayouts(world_, device, layouts.value());
        }
        if (pipelines.has_value()) {
            loadPipelines(world_, device, pipelines.value());
        }
        if (textures.has_value()) {
            loadTextures(world_, device, textures.value());
        }
        if (materials.has_value()) {
            loadMaterials(world_, device, materials.value());
        }
    }

    VkPipeline MaterialsModule::createMaterialPipeline(const MaterialPipelineDetails * mpd,
                                                       const FragmentShader * frag,
                                                       const VertexShader * vert,
                                                       VkPipelineLayout layout,
                                                       VkRenderPass rp,
                                                       uint32_t subpass)
    {
        VkGraphicsPipelineCreateInfo gpci{};
        gpci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

        VkPipelineDynamicStateCreateInfo pdsci{};
        pdsci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

        VkPipelineColorBlendStateCreateInfo pcbsci{};
        pcbsci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

        VkPipelineDepthStencilStateCreateInfo pdssci{};
        pdssci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

        VkPipelineMultisampleStateCreateInfo pmsci{};
        pmsci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

        VkPipelineRasterizationStateCreateInfo prsci{};
        prsci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

        VkPipelineViewportStateCreateInfo pvsci{};
        pvsci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

        VkPipelineInputAssemblyStateCreateInfo piasci{};
        piasci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

        VkPipelineVertexInputStateCreateInfo pvisci{};
        pvisci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        std::vector<VkPipelineColorBlendAttachmentState> attachments;
        std::vector<VkDynamicState> dynamicStates;

        piasci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        piasci.primitiveRestartEnable = VK_FALSE;

        pvsci.viewportCount = 1;
        pvsci.pViewports = nullptr;
        pvsci.scissorCount = 1;

        prsci.lineWidth = (mpd->lineWidth);
        prsci.polygonMode = (static_cast<VkPolygonMode>(mpd->fillMode));
        prsci.depthClampEnable = (mpd->depthClamp);
        prsci.rasterizerDiscardEnable = (false);
        prsci.cullMode = (static_cast<VkCullModeFlagBits>(mpd->cullMode));
        prsci.frontFace = (static_cast<VkFrontFace>(mpd->frontFace));
        pmsci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        shaderStages.push_back(
            VkPipelineShaderStageCreateInfo{
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                nullptr,
                0,
                VK_SHADER_STAGE_VERTEX_BIT,
                vert->shader->Handle(),
                "main",
                nullptr
            }
        );
        shaderStages.push_back(
            VkPipelineShaderStageCreateInfo{
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                nullptr,
                0,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                frag->shader->Handle(),
                "main",
                nullptr
            }
        );

        for (auto & mpa: mpd->blends) {
            auto & at = attachments.emplace_back();
            at.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            at.blendEnable = (mpa.enable);
            at.srcColorBlendFactor = (static_cast<VkBlendFactor>(mpa.sourceFactor));
            at.dstColorBlendFactor = (static_cast<VkBlendFactor>(mpa.destFactor));
            at.colorBlendOp = (static_cast<VkBlendOp>(mpa.colorBlendOp));
            at.srcAlphaBlendFactor = (static_cast<VkBlendFactor>(mpa.sourceAlphaFactor));
            at.dstAlphaBlendFactor = (static_cast<VkBlendFactor>(mpa.destAlphaFactor));
            at.alphaBlendOp = (static_cast<VkBlendOp>(mpa.alphaBlendOp));
        }

        pdssci.depthTestEnable = (mpd->depthWriteEnable);
        pdssci.depthWriteEnable = (mpd->depthTestEnable);
        pdssci.depthCompareOp = (static_cast<VkCompareOp>(mpd->depthCompareOp));
        pdssci.depthBoundsTestEnable = (false);
        pdssci.stencilTestEnable = (mpd->stencilTest);
        //        pdssci.front = ( {
        //          VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_NEVER, 0, 0, 0
        //    });
        //  pdssci.back = ( {
        //    VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_NEVER, 0, 0, 0
        //});
        pdssci.minDepthBounds = (mpd->minDepth);
        pdssci.maxDepthBounds = (mpd->maxDepth);

        dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
        dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);

        gpci.pInputAssemblyState = (&piasci);
        gpci.pViewportState = (&pvsci);
        gpci.pRasterizationState = (&prsci);
        gpci.pMultisampleState = (&pmsci);
        gpci.pDepthStencilState = (&pdssci);
        gpci.pColorBlendState = (&pcbsci);
        gpci.pVertexInputState = (&pvisci);
        gpci.pDynamicState = (&pdsci);
        gpci.layout = (layout);
        gpci.stageCount = static_cast<uint32_t>(shaderStages.size());
        gpci.pStages = shaderStages.data();
        gpci.renderPass = rp;
        gpci.subpass = subpass;

        std::vector<VkVertexInputBindingDescription> bindings;
        std::vector<VkVertexInputAttributeDescription> attributes;

        if (mpd->inputs.size() > 0) {
            uint32_t offset = 0;
            uint32_t loc = 0;

            for (auto & i: mpd->inputs) {
                if (i.inputType == RxAssets::MaterialPipelineInputType::eFloat) {
                    switch (i.count) {
                    case 1:
                        attributes.push_back({ loc++, 0, VK_FORMAT_R32_SFLOAT, offset });
                        offset += 4;
                        break;
                    case 2:
                        attributes.push_back({ loc++, 0, VK_FORMAT_R32G32_SFLOAT, offset });
                        offset += 8;
                        break;
                    case 3:
                        attributes.push_back({ loc++, 0, VK_FORMAT_R32G32B32_SFLOAT, offset });
                        offset += 12;
                        break;
                    case 4:
                        attributes.push_back({ loc++, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offset });
                        offset += 16;
                        break;
                    default: ;
                    }
                }
                if (i.inputType == RxAssets::MaterialPipelineInputType::eByte) {
                    switch (i.count) {
                    case 1:
                        attributes.push_back({ loc++, 0, VK_FORMAT_R8_UNORM, offset });
                        offset += 1;
                        break;
                    case 2:
                        attributes.push_back({ loc++, 0, VK_FORMAT_R8G8_UNORM, offset });
                        offset += 2;
                        break;
                    case 3:
                        attributes.push_back({ loc++, 0, VK_FORMAT_R8G8B8_UNORM, offset });
                        offset += 3;
                        break;
                    case 4:
                        attributes.push_back({ loc++, 0, VK_FORMAT_R8G8B8A8_UNORM, offset });
                        offset += 4;
                        break;
                    default: ;
                    }
                }
            }

            bindings.push_back({ 0, offset, VK_VERTEX_INPUT_RATE_VERTEX });
            pvisci.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
            pvisci.pVertexBindingDescriptions = bindings.data();
            pvisci.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
            pvisci.pVertexAttributeDescriptions = attributes.data();

        }
        pcbsci.attachmentCount = static_cast<uint32_t>(attachments.size());
        pcbsci.pAttachments = attachments.data();

        pdsci.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        pdsci.pDynamicStates = dynamicStates.data();

        auto device = engine_->getDevice();
        std::vector<VkPipeline> pipelines(1);

        auto rv = vkCreateGraphicsPipelines(device->getDevice(), nullptr, 1, &gpci, nullptr,
                                            pipelines.data());
        //auto rv = device->getDevice().createGraphicsPipeline(nullptr, gpci);
        assert(rv == VK_SUCCESS);
        if(rv != VK_SUCCESS) {
            spdlog::critical("Unable to create graphics pipeline");
        }

        return pipelines[0];
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
        auto device = engine_->getDevice();

        if (vert && frag && mpd) {
            if (mpd->stage == RxAssets::PipelineRenderStage::UI) {
                auto pl = createMaterialPipeline(
                    mpd, frag, vert, pll->layout, rp->uiRenderPass, rp->uiSubPass
                );
                e.setDeferred<GraphicsPipeline>(
                    {
                        std::make_shared<RxCore::Pipeline>(device, pl), rp->uiRenderPass,
                        rp->uiSubPass
                    }
                );
            }

            if (mpd->stage == RxAssets::PipelineRenderStage::Opaque) {
                auto pl = createMaterialPipeline(
                    mpd, frag, vert, pll->layout, rp->opaqueRenderPass,
                    rp->opaqueSubPass
                );
                e.setDeferred<GraphicsPipeline>(
                    {
                        std::make_shared<RxCore::Pipeline>(device, pl), rp->opaqueRenderPass,
                        rp->opaqueSubPass
                    }
                );
            }

            if (mpd->stage == RxAssets::PipelineRenderStage::Shadow) {
                auto pl = createMaterialPipeline(
                    mpd, frag, vert, pll->layout, rp->shadowRenderPass,
                    rp->shadowSubPass
                );
                e.setDeferred<GraphicsPipeline>(
                    {
                        std::make_shared<RxCore::Pipeline>(device, pl), rp->shadowRenderPass,
                        rp->shadowSubPass
                    }
                );
            }

            if (mpd->stage == RxAssets::PipelineRenderStage::Transparent) {
                auto pl = createMaterialPipeline(
                    mpd, frag, vert, pll->layout, rp->transparentRenderPass,
                    rp->transparentSubPass
                );
                e.setDeferred<GraphicsPipeline>(
                    {
                        std::make_shared<RxCore::Pipeline>(device, pl), rp->transparentRenderPass,
                        rp->transparentSubPass
                    }
                );
            }
        }
    }

    void MaterialsModule::createShaderMaterialData(ecs::EntityHandle e, DescriptorSet * ds)
    {
        auto res = world_->getResults(materialQuery);
        if (res.count() == 0) {
            return;
        }
        auto buffer = engine_->createStorageBuffer(res.count() * sizeof(MaterialShaderEntry));

        std::vector<MaterialShaderEntry> mv;
        std::vector<RxCore::CombinedSampler> ts;

        res.each<Material>(
            [&](ecs::EntityHandle e, Material * m)
            {
                m->sequence = static_cast<uint32_t>(mv.size());

                //auto me = mv.emplace_back();
                //me.roughness = m->roughness;
                auto te = m->materialTextures[0];

                auto tx = world_->get<MaterialImage>(te, true);
                auto sm = world_->get<Render::MaterialSampler>(te);

                //me.colorTextureIndex = ts.size();
                mv.push_back({static_cast<uint32_t>(ts.size()), m->roughness});
                ts.push_back({sm->sampler, tx->imageView});
            }
        );

        buffer->map();
        buffer->update(mv.data(), mv.size() * sizeof(MaterialShaderEntry));
        buffer->unmap();

        ds->ds->updateDescriptor(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, buffer);
        ds->ds->updateDescriptor(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, ts);

        e.addDeferred<MaterialDescriptor>();
    }
}
