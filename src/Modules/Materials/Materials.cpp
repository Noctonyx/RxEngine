#include "Materials.h"
#include "RXCore.h"
#include "Modules/Renderer/Renderer.hpp"

namespace RxEngine
{
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
}
