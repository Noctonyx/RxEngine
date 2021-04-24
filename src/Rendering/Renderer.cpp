#include <memory>
#include <algorithm>
#include <tuple>
#include "Renderer.hpp"
#include "Vulkan/Surface.hpp"
#include "Vulkan/SwapChain.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/Queue.hpp"
#include <utility>
#include "Vulkan/FrameBuffer.h"
#include "Subsystems/Lighting.h"
#include "robin_hood.h"
#include "DirectXCollision.h"
#include "Modules/Render.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/Shader.h"

using namespace DirectX;

namespace RxEngine
{
    Renderer::Renderer(vk::Device device,
                       ecs::World * world,
                       vk::Format imageFormat,
                       EngineMain * engine)
        : DeviceObject(device)
        , Module(world, engine)
        , imageFormat_(imageFormat)
        , shadowImagesChanged(true)
        , poolTemplate(
            {
                {vk::DescriptorType::eCombinedImageSampler, 15000},
                {vk::DescriptorType::eUniformBufferDynamic, 600},
                {vk::DescriptorType::eStorageBuffer, 600},
                {vk::DescriptorType::eUniformBuffer, 800}
            },
            400)
    //, world_(world)
    //, query_(*world, "!RxEngine.Render.Pipeline")
    {
        //auto q = world_->query<const Render::MaterialPipelineDetails>();

        // lightingManager_ = std::make_shared<Lighting>();
    }

    Renderer::~Renderer() = default;

    void Renderer::startup()
    {
        createRenderPass();
        createDepthRenderPass();
        createPipelineLayout();

        graphicsCommandPool_ = RxCore::Device::Context()->CreateGraphicsCommandPool();

        vk::QueryPoolCreateInfo qpci;

        qpci.queryType = vk::QueryType::eTimestamp;
        qpci.queryCount = 128;

        queryPool_ = RxCore::Device::VkDevice().createQueryPool(qpci);

        for (uint32_t i = 0; i < 20; i++) {
            auto b = RxCore::iVulkan()->createBuffer(
                vk::BufferUsageFlagBits::eStorageBuffer,
                VMA_MEMORY_USAGE_CPU_TO_GPU,
                20000 * sizeof(IndirectDrawInstance)
            );

            b->getMemory()->map();
            auto ds = RxCore::JobManager::threadData().getDescriptorSet(
                poolTemplate, dsLayouts[2]);
            ds->updateDescriptor(0, vk::DescriptorType::eStorageBuffer, b);
            instanceBuffers.push_back(b);
            instanceBUfferDS.push_back(ds);
        }

        world_->createSystem("Renderer:Render")
              .inGroup("Pipeline:Render")
              .after<AcquireImage>()
              .before<PresentImage>()
              .withStream<MainRenderImageInput>()
              .execute<MainRenderImageInput>(
                  [this](ecs::World * world, const MainRenderImageInput * mri)
                  {
                      render(mri->imageView,
                             mri->extent,
                             {mri->imageAvailableSempahore},
                             {vk::PipelineStageFlagBits::eColorAttachmentOutput},
                             mri->finishRenderSemaphore);

                      world->getStream<MainRenderImageOutput>()
                           ->add<MainRenderImageOutput>(
                               {mri->imageView, mri->finishRenderSemaphore});

                      return true;
                  });

        world_->createSystem("Renderer:CleanLastFrame")
              .inGroup("Pipeline:PreRender")
              .execute([](ecs::World *)
                  {
                      OPTICK_EVENT("Release Previous Frame Resource")
                      RxCore::Device::Context()->graphicsQueue_->ReleaseCompleted();
                  }
              );

        world_->createSystem("Renderer:Pipelines")
              .inGroup("Pipeline:PreFrame")
              .withQuery<Render::MaterialPipelineDetails>()
              .without<Render::HasPipeline>()
              .withRelation<Render::UsesVertexShader, Render::VertexShader>()
              .withRelation<Render::UsesFragmentShader, Render::FragmentShader>()
              .each<Render::MaterialPipelineDetails, Render::FragmentShader, Render::VertexShader>(
                  [this](ecs::EntityHandle e,
                         const Render::MaterialPipelineDetails * mpd,
                         const Render::FragmentShader * frag,
                         const Render::VertexShader * vert)
                  {
                      if (vert && frag && mpd) {
                          if (mpd->stage == RxAssets::PipelineRenderStage::UI) {
                              auto pl = createUiMaterialPipeline(mpd, frag, vert, renderPass_, 0);
                              e.setDeferred<Render::UiPipeline>({
                                  std::make_shared<RxCore::Pipeline>(pl), renderPass_, 0
                              });
                              e.addDeferred<Render::HasPipeline>();
                          }
                      }
                  });
    }

    void Renderer::createDepthRenderPass()
    {
        std::vector<vk::AttachmentDescription> ad = {
            {
                {},
                RxCore::Device::Context()->GetDepthFormat(true),
                vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
                vk::AttachmentLoadOp::eDontCare,
                vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eDepthStencilReadOnlyOptimal
            }
        };

        std::vector<vk::AttachmentReference> input_attachments = {};
        std::vector<vk::AttachmentReference> color_attachments = {};
        std::vector<vk::AttachmentReference> resolve_attachments = {};
        vk::AttachmentReference depth_attachment = {
            0, vk::ImageLayout::eDepthStencilAttachmentOptimal
        };
        std::vector<uint32_t> preserve_attachments = {};

        std::vector<vk::SubpassDescription> sp = {
            {
                {},
                vk::PipelineBindPoint::eGraphics,
                input_attachments,
                color_attachments,
                resolve_attachments,
                &depth_attachment,
                {}
            }
        };

        std::vector<vk::SubpassDependency> spd = {
            {
                VK_SUBPASS_EXTERNAL,
                0,
                vk::PipelineStageFlagBits::eFragmentShader,
                vk::PipelineStageFlagBits::eEarlyFragmentTests,
                vk::AccessFlagBits::eShaderRead,
                vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                vk::DependencyFlagBits::eByRegion
            },
            {
                0,
                VK_SUBPASS_EXTERNAL,
                vk::PipelineStageFlagBits::eLateFragmentTests,
                vk::PipelineStageFlagBits::eFragmentShader,
                vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                vk::AccessFlagBits::eShaderRead,
                vk::DependencyFlagBits::eByRegion
            }
        };

        vk::RenderPassCreateInfo rpci;
        rpci.setAttachments(ad).setSubpasses(sp).setDependencies(spd);

        auto rph = device_.createRenderPass(rpci);
        depthRenderPass_ = rph;
    }

    void Renderer::createRenderPass()
    {
        std::vector<vk::AttachmentDescription> ads{
            {
                {},
                imageFormat_,
                vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
                vk::AttachmentLoadOp::eDontCare,
                vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::ePresentSrcKHR // ! important
            },
            {
                {},
                RxCore::Device::Context()->GetDepthFormat(false),
                vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eDontCare,
                vk::AttachmentLoadOp::eDontCare,
                vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eDepthStencilAttachmentOptimal
            }
        };

        std::vector<vk::AttachmentReference> input_attachments = {};
        std::vector<vk::AttachmentReference> color_attachments{
            {0, vk::ImageLayout::eColorAttachmentOptimal}
        };
        std::vector<vk::AttachmentReference> resolve_attachments = {};
        vk::AttachmentReference depth_attachment{
            1,
            vk::ImageLayout::eDepthStencilAttachmentOptimal
        };
        std::vector<uint32_t> preserve_attachments = {};

        std::vector<vk::SubpassDescription> sp{
            {
                {},
                vk::PipelineBindPoint::eGraphics,
                input_attachments,
                color_attachments,
                resolve_attachments,
                &depth_attachment,
                {}
            }
        };

        std::vector<vk::SubpassDependency> spd = {
            {
                VK_SUBPASS_EXTERNAL,
                0,
                vk::PipelineStageFlagBits::eColorAttachmentOutput,
                vk::PipelineStageFlagBits::eColorAttachmentOutput,
                {},
                vk::AccessFlagBits::eColorAttachmentRead |
                vk::AccessFlagBits::eColorAttachmentWrite,
                vk::DependencyFlagBits::eByRegion
            }
        };
        vk::RenderPassCreateInfo rpci;
        rpci.setAttachments(ads).setSubpasses(sp).setDependencies(spd);

        auto rph = device_.createRenderPass(rpci);

        renderPass_ = rph;
    }

    void Renderer::ensureMaterialPipelinesExist()
    {
#if 0
        query_.each([&](flecs::entity e, const Render::MaterialPipelineDetails & d)
        {
            if (d.stage == RxAssets::PipelineRenderStage::Opaque) {
                e.add<Render::OpaquePipeline>();
            }
            if (d.stage == RxAssets::PipelineRenderStage::Shadow) {
                e.add<Render::ShadowPipeline>();
            }
            if (d.stage == RxAssets::PipelineRenderStage::UI) {
                e.add<Render::UiPipeline>();
            }
            if (d.stage == RxAssets::PipelineRenderStage::Transparent) {
                e.add<Render::TransparentPipeline>();
            }
        });
#endif
#if 0
        materialManager_->ensureOpaqueMaterialPipelinesExist(pipelineLayout, renderPass_, 0);
        materialManager_->ensureShadowMaterialPipelinesExist(pipelineLayout, depthRenderPass_, 0);
#endif
    }

    std::shared_ptr<const std::vector<RenderEntity>> Renderer::finishUpEntityJobs(
        const std::vector<std::shared_ptr<RxCore::Job<std::vector<RenderEntity>>>> & entityJobs)
    {
        OPTICK_EVENT();
        std::vector<RenderEntity> entities;

        std::vector<std::vector<RenderEntityInstance>> entityInstances;
        std::unordered_map<uint32_t, uint32_t> entityMap;

        {
            OPTICK_EVENT("Build Map")

            for (auto & j: entityJobs) {
                j->waitComplete();
                auto res = j->result;

                if (!res.empty()) {
                    entities.reserve(res.size());
                    {
                        OPTICK_EVENT("Insert")

                        entities.insert(entities.end(), res.begin(), res.end());
                    }
                }
            }
        }
        {
            OPTICK_EVENT("Copy vector")
            return std::make_shared<const std::vector<RenderEntity>>(std::move(entities));
        }
    }

    void Renderer::render(
        //const std::shared_ptr<RenderCamera> & renderCamera,
        //const std::vector<IRenderable *> & subsystems,
        vk::ImageView imageView,
        vk::Extent2D extent,
        std::vector<vk::Semaphore> waitSemaphores,
        std::vector<vk::PipelineStageFlags> waitStages,
        vk::Semaphore completeSemaphore)
    {
        OPTICK_EVENT()

        const auto start_time = std::chrono::high_resolution_clock::now();

        ensureDepthBufferExists(extent);
        ensureShadowImages(4096, NUM_CASCADES);

        //   renderCamera->readyCameraFrame();
        //        lightingManager_->setup(renderCamera->getCamera());

        //  updateDescriptorSet0(renderCamera);
        //entityManager_->ensureDescriptors(poolTemplate, dsLayouts[1]);

        std::vector<std::shared_ptr<RxCore::Job<RenderResponse>>> shadow_jobs;
        std::vector<std::shared_ptr<RxCore::Job<RenderResponse>>> ui_jobs;
        std::vector<std::shared_ptr<RxCore::Job<std::vector<RenderEntity>>>> entity_jobs;

        {
            OPTICK_EVENT("Get Subsystem Entities")
#if 0
            for (auto * subsystem: subsystems) {
                auto job = RxCore::CreateJob<std::vector<RenderEntity>>(
                    [=]
                    {
                        OPTICK_EVENT("Render Subsystem General")
                        return subsystem->getRenderEntities();
                    });
                job->schedule();
                entity_jobs.push_back(job);
            }
#endif
        }

        std::vector<ShadowCascade> cascades;
        {
#if 0
            if (lightingManager_) {
                lightingManager_->getCascadeData(cascades);
                lightingManager_->setShadowMap(wholeShadowMapView_);
            }
            createUiJobs(ui_jobs, subsystems, extent.width, extent.height);
#endif
        }
#if 0
        {
            OPTICK_EVENT("Release Previous Frame Resource")
            RxCore::Device::Context()->graphicsQueue_->ReleaseCompleted();
        }
#endif
        ensureMaterialPipelinesExist();

        //std::shared_ptr<const std::vector<RenderEntity>> entity_ptr;

        //        entity_ptr = finishUpEntityJobs(entity_jobs);
#if 0
        if (!cascades.empty()) {
            shadow_jobs.resize(cascades.size());
            for (uint32_t i = 0; i < cascades.size(); i++) {

                shadow_jobs[i] = RxCore::CreateJob<RenderResponse>(
                    [=]() -> RenderResponse
                    {
                        return renderShadow(
                            {4096, 4096},
                            XMLoadFloat4x4(&(cascades[i].viewProjMatrix)),
                            XMLoadFloat4x4(&(cascades[i].viewMatrix)),
                            cascades[i].boBox,
                            i,
                            entity_ptr);
                    });
                shadow_jobs[i]->schedule();
            }
        }

        auto opaque_job = RxCore::CreateJob<RenderResponse>(
            [=]() -> RenderResponse
            {
                return renderOpaque(extent, renderCamera->getCamera(), entity_ptr);
            });
        opaque_job->schedule();

        entity_ptr.reset();
#endif

        auto buf = graphicsCommandPool_->GetPrimaryCommandBuffer();
#if 0
        {
            OPTICK_EVENT("Wait for Render Jobs", Optick::Category::Wait)
            //waitAndFinishJobs(opaque_jobs, ERenderSequence::RenderSequenceOpaque, buf);
            waitAndFinishJobs(ui_jobs, ERenderSequence::RenderSequenceUi, buf);

            for (uint32_t i = 0; i < shadow_jobs.size(); i++) {
                shadow_jobs[i]->waitComplete();
                auto res = shadow_jobs[i]->result;
                if (res.has_value()) {
                    buf->addSecondaryBuffer(res.value(), static_cast<uint16_t>(1000 + i));
                }
            }
        }
        opaque_job->waitComplete();
        auto res = opaque_job->result;
        if (res.has_value()) {
            buf->addSecondaryBuffer(res.value(), ERenderSequence::RenderSequenceOpaque);
        }
#endif
        std::shared_ptr<RxCore::FrameBuffer> frame_buffer;
        frame_buffer = createRenderFrameBuffer(imageView, extent);
        {
            OPTICK_EVENT("Build Primary Buffer")
            buf->begin();
            OPTICK_GPU_CONTEXT(buf->Handle())
            {
                buf->Handle().resetQueryPool(queryPool_, 0, 128);
                buf->Handle().writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, queryPool_, 0);

                std::vector<vk::ClearValue> depth_clear_values = {vk::ClearValue({1.0f, ~0u})};

                {
                    OPTICK_GPU_EVENT("Shadow RenderPass")
                    for (uint32_t i = 0; i < NUM_CASCADES; i++) {
                        buf->beginRenderPass(
                            depthRenderPass_, cascadeFrameBuffers_[i],
                            vk::Extent2D{4096, 4096}, depth_clear_values);
                        {
                            OPTICK_EVENT("Execute Secondaries")

                            buf->executeSecondaries(static_cast<uint16_t>(1000 + i));
                        }
                        buf->EndRenderPass();
                    }
                }
                std::vector<vk::ClearValue> clear_values = {
                    vk::ClearValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}),
                    vk::ClearValue({1.0f, ~0u})
                };
                {
                    OPTICK_GPU_EVENT("RenderPass")
                    buf->beginRenderPass(renderPass_, frame_buffer, extent, clear_values);
                    {
                        OPTICK_EVENT("Execute Secondaries")

                        buf->executeSecondaries(RenderSequenceOpaque);
                        buf->executeSecondaries(RenderSequenceUi);
                    }
                    buf->EndRenderPass();
                }

                buf->Handle().writeTimestamp(
                    vk::PipelineStageFlagBits::eBottomOfPipe, queryPool_,
                    1);
            }
            buf->end();
        }
#if 0
        lightingManager_->teardown();
#endif
        // frameBufferIndex_ = (frameBufferIndex_ + 1) % imageCount_;
        {
            OPTICK_EVENT("GPU Submit", Optick::Category::Rendering)
            RxCore::Device::Context()->graphicsQueue_->Submit(
                {buf}, std::move(waitSemaphores), std::move(waitStages), {completeSemaphore});
        }

        const auto end_time = std::chrono::high_resolution_clock::now();
        cpuTime = std::chrono::duration<double, std::milli>((end_time - start_time)).count();
    }

    void Renderer::updateDescriptorSet0(const std::shared_ptr<RenderCamera> & renderCamera)
    {
        OPTICK_EVENT()
#if 0
        if (shadowImagesChanged || materialManager_->getChangeSequence() != materialManagerSequence_
        ) {
            ds0_ = RxCore::JobManager::threadData().getDescriptorSet(
                poolTemplate,
                dsLayouts[0], {1000});
            renderCamera->updateDescriptor(ds0_, 0);
            lightingManager_->updateDescriptor(ds0_, 1);
            ds0_->updateDescriptor(
                2,
                vk::DescriptorType::eCombinedImageSampler,
                wholeShadowMapView_,
                vk::ImageLayout::eDepthStencilReadOnlyOptimal,
                shadowSampler_);
            materialManager_->updateMaterialBufferDescriptor(ds0_, 3);
            materialManager_->updateTextureDescriptor(ds0_, 4);

            shadowImagesChanged = false;
            materialManagerSequence_ = materialManager_->getChangeSequence();
        } else {
            ds0_->setDescriptorOffset(0, renderCamera->getDescriptorOffset());
            ds0_->setDescriptorOffset(1, lightingManager_->getDescriptorOffset());
        }
#endif
    }

    std::shared_ptr<RxCore::FrameBuffer> Renderer::createRenderFrameBuffer(
        const vk::ImageView & imageView,
        const vk::Extent2D & extent) const
    {
        OPTICK_EVENT("Create Framebuffer")
        std::vector<vk::ImageView> attachments = {imageView, depthBufferView_->handle};

        auto frame_buffer = std::make_shared<RxCore::FrameBuffer>(
            device_.createFramebuffer(
                {{}, renderPass_, attachments, extent.width, extent.height, 1}));

        return frame_buffer;
    }

#if 0
    void Renderer::ensureFrameBufferSize(const vk::ImageView & imageView, const vk::Extent2D & extent)
    {
        if (frameBuffers_[frameBufferIndex_].extent != extent) {

            std::vector<vk::ImageView> attachments = {
                imageView,
                depthBufferView_->handle
            };
            //auto fbh = renderPass->CreateFrameBuffer(attachments, extent.width, extent.height);

            vk::FramebufferCreateInfo fbci{
                {},
                *renderPass_,
                attachments, extent.width, extent.height, 1
            };

            frameBuffers_[frameBufferIndex_].extent = extent;
            frameBuffers_[frameBufferIndex_].fb = std::make_shared<FrameBuffer>(iDevice().createFramebuffer(fbci));
            //frameBuffers_[frameBufferIndex].second = fbh;
        }
    }
#endif

    void Renderer::ensureDepthBufferExists(vk::Extent2D & extent)
    {
        if (extent != bufferExtent_) {
            depthBuffer_ = RxCore::Device::Context()->createImage(
                RxCore::Device::Context()->GetDepthFormat(false),
                {extent.width, extent.height, 1},
                1, 1,
                vk::ImageUsageFlagBits::eDepthStencilAttachment);

            depthBufferView_ = depthBuffer_->createImageView(
                vk::ImageViewType::e2D,
                vk::ImageAspectFlagBits::eDepth,
                0,
                1);
            bufferExtent_ = extent;
        }
    }

    void Renderer::shutdown()
    {
        world_->deleteSystem(world_->lookup("Renderer:Render").id);

        RxCore::iVulkan()->WaitIdle();
        device_.destroyQueryPool(queryPool_);
        // frameBuffers_.clear();
        depthBufferView_.reset();
        depthBuffer_.reset();
        graphicsCommandPool_.reset();
        //lightingManager_.reset();

        cascadeViews_.clear();
        cascadeFrameBuffers_.clear();
        shadowMap_.reset();
        wholeShadowMapView_.reset();

        ds0_.reset();

        device_.destroyRenderPass(renderPass_);
        device_.destroyRenderPass(depthRenderPass_);
    }

    RenderStage Renderer::getSequenceRenderStage(ERenderSequence seq) const
    {
        if (seq == RenderSequenceOpaque || seq == RenderSequenceUi) {
            return RenderStage{renderPass_, 0};
        }
        if (seq == RenderSequenceShadowPass) {
            return RenderStage{depthRenderPass_, 0};
        }
        assert(false);
        return {};
    }

    void Renderer::ensureShadowImages(uint32_t
                                      shadowMapSize,
                                      uint32_t numCascades
    )
    {
        if (
            shadowMap_ && shadowMap_
                          ->extent_.width == shadowMapSize) {
            return;
        }
        shadowImagesChanged = true;
        shadowMap_ = RxCore::iVulkan()->createImage(
            RxCore::Device::Context()->GetDepthFormat(false),
            {shadowMapSize, shadowMapSize, 1},
            1,
            numCascades,
            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled);

        wholeShadowMapView_ = shadowMap_->createImageView(
            vk::ImageViewType::e2DArray, vk::ImageAspectFlagBits::eDepth, 0, numCascades);

        cascadeViews_.
            resize(numCascades);
        cascadeFrameBuffers_.
            resize(numCascades);

        for (
            uint32_t i = 0;
            i < numCascades;
            ++i) {
            cascadeViews_[i] = shadowMap_->
                createImageView(
                    vk::ImageViewType::e2DArray,
                    vk::ImageAspectFlagBits::eDepth, i,
                    1);

            std::vector<vk::ImageView> attachments = {cascadeViews_[i]->handle};

            cascadeFrameBuffers_[i] =
                std::make_shared<RxCore::FrameBuffer>(
                    device_
                    .createFramebuffer(
                        {
                            {
                            },
                            depthRenderPass_, attachments, shadowMapSize, shadowMapSize, 1
                        }));
        }

        if (!shadowSampler_) {
            vk::SamplerCreateInfo sci;
            sci.
                setMagFilter(vk::Filter::eLinear)
                .
                setMinFilter(vk::Filter::eLinear)
                .
                setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
                .
                setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
                .setMaxLod(5.0f)
                .
                setBorderColor(vk::BorderColor::eFloatOpaqueWhite);

            shadowSampler_ = RxCore::iVulkan()->createSampler(sci);
        }
    }

    void Renderer::setScissorAndViewport(
        vk::Extent2D extent,
        std::shared_ptr<RxCore::SecondaryCommandBuffer> buf,
        bool flipY) const
    {
        buf->setScissor(
            {
                {0, 0},
                {extent.width, extent.height}
            });
        buf->setViewport(
            .0f, flipY ? static_cast<float>(extent.height) : 0.0f, static_cast<float>(extent.width),
            flipY ? -static_cast<float>(extent.height) : static_cast<float>(extent.height), 0.0f,
            1.0f);
    }

    vk::Pipeline Renderer::createUiMaterialPipeline(const Render::MaterialPipelineDetails * mpd,
                                                    const Render::FragmentShader * frag,
                                                    const Render::VertexShader * vert,
                                                    vk::RenderPass rp,
                                                    uint32_t subpass)
    {
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
            .setLayout(pipelineLayout)
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

        auto rv = device_.createGraphicsPipeline(nullptr, gpci);
        assert(rv.result == vk::Result::eSuccess);

        return rv.value;
    }

    void Renderer::createPipelineLayout()
    {
        std::vector<vk::DescriptorSetLayoutBinding> set_0{
            {
                0, vk::DescriptorType::eUniformBufferDynamic, 1,
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment
            },
            {
                1, vk::DescriptorType::eUniformBufferDynamic, 1,
                vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex
            },
            {
                2, vk::DescriptorType::eCombinedImageSampler, 1,
                vk::ShaderStageFlagBits::eFragment
            },
            {
                3, vk::DescriptorType::eStorageBuffer, 1,
                vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex
            },
            {
                4, vk::DescriptorType::eCombinedImageSampler, 4096,
                vk::ShaderStageFlagBits::eFragment
            }
        };

        std::vector<vk::DescriptorBindingFlags> set0_binding_flags = {
            {},
            {},
            {},
            {},
            vk::DescriptorBindingFlagBits::eVariableDescriptorCount |
            vk::DescriptorBindingFlagBits::ePartiallyBound |
            vk::DescriptorBindingFlagBits::eUpdateAfterBind
        };

        std::vector<vk::DescriptorSetLayoutBinding> set_1 = {
            {
                0, vk::DescriptorType::eStorageBuffer, 1,
                vk::ShaderStageFlagBits::eVertex
            },
        };

        std::vector<vk::DescriptorSetLayoutBinding> set_2 = {
            {
                0, vk::DescriptorType::eStorageBuffer, 1,
                vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex
            }
        };

        vk::DescriptorSetLayoutBindingFlagsCreateInfo dslbfc_0{};
        dslbfc_0.setBindingFlags(set0_binding_flags);

        //vk::DescriptorSetLayoutCreateInfo dslci0{{}, set_0};
        vk::DescriptorSetLayoutCreateInfo dslci0{
            vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool, set_0
        };
        vk::DescriptorSetLayoutCreateInfo dslci1{{}, set_1};
        vk::DescriptorSetLayoutCreateInfo dslci2{{}, set_2};

        dslci0.setPNext(&dslbfc_0);

        dsLayouts.resize(3);

        auto device = RxCore::iVulkan();

        dsLayouts[0] = device->createDescriptorSetLayout(dslci0);
        dsLayouts[1] = device->createDescriptorSetLayout(dslci1);
        dsLayouts[2] = device->createDescriptorSetLayout(dslci2);
        //dsLayouts[2] = device->createDescriptorSetLayout(dslci2);

        vk::PipelineLayoutCreateInfo plci;
        std::vector<vk::PushConstantRange> pcrs{
            {vk::ShaderStageFlagBits::eVertex, 0, 4}
        };
        plci.setPushConstantRanges(pcrs);
        plci.setSetLayouts(dsLayouts);

        pipelineLayout = device->createPipelineLayout(plci);
    }

    void Renderer::cullEntitiesProj(
        const XMMATRIX & proj,
        const XMMATRIX & view,
        const std::shared_ptr<const std::vector<RenderEntity>> & entities,
        std::vector<uint32_t> & selectedEntities)
    {
        OPTICK_EVENT()
        BoundingFrustum frustum(proj, true), viewFrustum;

        frustum.Transform(viewFrustum, XMMatrixInverse(nullptr, view));

        auto ec = static_cast<uint32_t>(entities->size());

        selectedEntities.reserve(ec);
        auto & E = *entities;

        for (uint32_t i = 0; i < ec; i++) {
            if (viewFrustum.Intersects(E[i].boundsSphere)) {
                //} frustum.isSphereVisible(E[i].boundsSphere)) {
                selectedEntities.insert(selectedEntities.end(), i);
            }
        }
        OPTICK_TAG("Before Cull", entities->size())
        OPTICK_TAG("Select from Cull", selectedEntities.size())
    }


    void Renderer::cullEntitiesOrtho(
        const BoundingOrientedBox & cullBox,
        const std::shared_ptr<const std::vector<RenderEntity>> & entities,
        std::vector<uint32_t> & selectedEntities)
    {
        OPTICK_EVENT()

        auto ec = static_cast<uint32_t>(entities->size());

        selectedEntities.reserve(ec);
        auto & E = *entities;

        for (uint32_t i = 0; i < ec; i++) {
            if (cullBox.Intersects(E[i].boundsSphere)) {
                //} frustum.isSphereVisible(E[i].boundsSphere)) {
                selectedEntities.insert(selectedEntities.end(), i);
            }
        }
        OPTICK_TAG("Before Cull", entities->size())
        OPTICK_TAG("Select from Cull", selectedEntities.size())
    }

    float Renderer::getSphereSize(
        const XMMATRIX & projView,
        const XMVECTOR & viewRight,
        const BoundingSphere & sphere)
    {
        auto p1 = XMLoadFloat3(&sphere.Center);
        auto p2 = XMVectorAdd(
            p1,
            XMVectorScale(viewRight, sphere.Radius)
        );

        p1 = XMVector3TransformCoord(p1, projView);
        p2 = XMVector3TransformCoord(p2, projView);

        return XMVectorGetX(XMVectorAbs(XMVectorSubtract(p1, p2))) / 2.0f;
    }
} // namespace RXCore
