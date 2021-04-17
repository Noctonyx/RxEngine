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

using namespace DirectX;

namespace RxEngine
{
    Renderer::Renderer(vk::Device device, World * world)
        : DeviceObject(device)
        , shadowImagesChanged(true)
        , materialManagerSequence_(99999999)
        , poolTemplate(
            {
                {vk::DescriptorType::eCombinedImageSampler, 15000},
                {vk::DescriptorType::eUniformBufferDynamic, 600},
                {vk::DescriptorType::eStorageBuffer, 600},
                {vk::DescriptorType::eUniformBuffer, 800}
            },
            400)
        , world_(world)
        //, query_(*world, "!RxEngine.Render.Pipeline")
    {
        //auto q = world_->query<const Render::MaterialPipelineDetails>();

        lightingManager_ = std::make_shared<Lighting>();
    }

    Renderer::~Renderer() = default;

    void Renderer::startup(vk::Format imageFormat)
    {
        imageFormat_ = imageFormat;

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
        materialManager_->ensureOpaqueMaterialPipelinesExist(pipelineLayout, renderPass_, 0);
        materialManager_->ensureShadowMaterialPipelinesExist(pipelineLayout, depthRenderPass_, 0);
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
        const std::shared_ptr<RenderCamera> & renderCamera,
        const std::vector<IRenderable *> & subsystems,
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

        renderCamera->readyCameraFrame();
        lightingManager_->setup(renderCamera->getCamera());

        updateDescriptorSet0(renderCamera);
        entityManager_->ensureDescriptors(poolTemplate, dsLayouts[1]);

        std::vector<std::shared_ptr<RxCore::Job<RenderResponse>>> shadow_jobs;
        std::vector<std::shared_ptr<RxCore::Job<RenderResponse>>> ui_jobs;
        std::vector<std::shared_ptr<RxCore::Job<std::vector<RenderEntity>>>> entity_jobs;

        {
            OPTICK_EVENT("Get Subsystem Entities")
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
        }

        std::vector<ShadowCascade> cascades;
        {
            if (lightingManager_) {
                lightingManager_->getCascadeData(cascades);
                lightingManager_->setShadowMap(wholeShadowMapView_);
            }
            createUiJobs(ui_jobs, subsystems, extent.width, extent.height);
        }

        {
            OPTICK_EVENT("Release Previous Frame Resource")
            RxCore::Device::Context()->graphicsQueue_->ReleaseCompleted();
        }

        ensureMaterialPipelinesExist();

        std::shared_ptr<const std::vector<RenderEntity>> entity_ptr;

        entity_ptr = finishUpEntityJobs(entity_jobs);

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

        auto buf = graphicsCommandPool_->GetPrimaryCommandBuffer();

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
        lightingManager_->teardown();
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
        RxCore::iVulkan()->WaitIdle();
        device_.destroyQueryPool(queryPool_);
        // frameBuffers_.clear();
        depthBufferView_.reset();
        depthBuffer_.reset();
        graphicsCommandPool_.reset();
        lightingManager_.reset();

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

    void Renderer::createUiJobs(
        std::vector<std::shared_ptr<RxCore::Job<RenderResponse>>> & jobs,
        const std::vector<IRenderable *> & subsystems,
        const uint32_t width,
        const uint32_t height)
    {
        OPTICK_EVENT("Render UI")
        for (auto * subsystem: subsystems) {
            if (subsystem->hasRenderUi()) {
                auto j = RxCore::CreateJob<RenderResponse>(
                    [=]
                    {
                        OPTICK_EVENT("Render Subsystem Ui")
                        return subsystem->renderUi(RenderStage{renderPass_, 0}, width, height);
                    });
                j->schedule();
                jobs.push_back(j);
            }
        }
    }

    void Renderer::waitAndFinishJobs(
        std::vector<std::shared_ptr<RxCore::Job<RenderResponse>>> & jobs,
        uint16_t seq,
        std::shared_ptr<RxCore::PrimaryCommandBuffer> & buf)
    {
        for (auto & j: jobs) {
            j->waitComplete();
            auto res = j->result;
            if (res.has_value()) {
                buf->addSecondaryBuffer(res.value(), seq);
            }
        }
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

    void Renderer::setLightingManager(std::shared_ptr<Lighting> lm)
    {
        lightingManager_ = std::move(lm);
    }

    void Renderer::setMaterialManager(MaterialManager * materialManager)
    {
        materialManager_ = materialManager;
    }

    void Renderer::setEntityManager(EntityManager * entityManager)
    {
        entityManager_ = entityManager;
    }

    void IRenderable::setScissorAndViewPort(
        const std::shared_ptr<RxCore::CommandBuffer> & buf,
        uint32_t width,
        uint32_t height,
        bool flipY)
    {
        buf->setScissor(
            {
                {0, 0},
                {width, height}
            });
        buf->setViewport(
            .0f, flipY ? static_cast<float>(height) : 0.0f, static_cast<float>(width),
            flipY ? -static_cast<float>(height) : static_cast<float>(height), 0.0f,
            1.0f);
    }

    void Renderer::renderIndirectDraws(
        IndirectDrawSet ids,
        const std::shared_ptr<RxCore::SecondaryCommandBuffer> & buf) const
    {
        OPTICK_EVENT()
        MaterialPipelineId current_pipeline{};
        MeshBundle * prevBundle = nullptr;

        for (auto & h: ids.headers) {
            OPTICK_EVENT("IDS Header")
            if (h.commandCount == 0) {
                continue;
            }
            {
                OPTICK_EVENT("Set Pipeline and buffers")
                if (h.pipelineId != current_pipeline) {
                    // Check the pipeline has been created (no cereate it earlier)
                    auto & pl = materialManager_->getMaterialPipeline(h.pipelineId);
                    // bind the pipeline
                    buf->BindPipeline(pl.pipeline);
                    current_pipeline = h.pipelineId;
                }
                if (h.bundle != prevBundle) {
                    OPTICK_EVENT("Bind Bundle")
                    {
                        if (h.bundle->isUseDescriptor()) {
                            OPTICK_EVENT("Bind Ds")
                            buf->BindDescriptorSet(1, h.bundle->getDescriptorSet());
                        } else {
                            OPTICK_EVENT("Bind VB")
                            buf->BindVertexBuffer(h.bundle->getVertexBuffer());
                        }
                    }
                    {
                        OPTICK_EVENT("Bind IB")
                        buf->BindIndexBuffer(h.bundle->getIndexBuffer());
                    }
                    // bind bundle descriptorSet
                    prevBundle = h.bundle;
                }
            }
            {
                OPTICK_EVENT("Draw Indexed")
                for (uint32_t i = 0; i < h.commandCount; i++) {
                    auto & c = ids.commands[i + h.commandStart];
                    buf->DrawIndexed(
                        c.indexCount, c.instanceCount, c.indexOffset, c.vertexOffset,
                        c.instanceOffset);
                }
            }
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

    RenderResponse Renderer::renderOpaque(
        vk::Extent2D extent,
        const std::shared_ptr<Camera> & camera,
        std::shared_ptr<const std::vector<RenderEntity>> entities)
    {
        OPTICK_EVENT()

        std::vector<uint32_t> visible_entity_index;
        std::vector<uint32_t> drawing_entity_index;

        cullEntitiesProj(
            XMLoadFloat4x4(&camera->cameraShaderData.projection),
            XMLoadFloat4x4(&camera->cameraShaderData.view),
            entities,
            visible_entity_index);

        MaterialPipelineId prev_pipeline;
        EntityId prev_entity_id;
        {
            OPTICK_EVENT("Select matching Pipeline")
            for (auto & i: visible_entity_index) {
                auto & e = (*entities)[i];
                MaterialPipelineId pl;
                if (e.entityId == prev_entity_id) {
                    pl = prev_pipeline;
                } else {
                    pl = entityManager_->getEntityOpaquePipeline(e.entityId);
                    prev_pipeline = pl;
                    prev_entity_id = e.entityId;
                }
                if (pl.valid()) {
                    drawing_entity_index.insert(drawing_entity_index.end(), i);
                }
            }
        }

        if (drawing_entity_index.empty()) {
            return {};
        }

        return renderSelectedEntities(
            extent,
            XMLoadFloat4x4(&camera->projView),
            XMLoadFloat4x4(&(camera->cameraShaderData.view)),
            entities,
            drawing_entity_index,
            1,
            true,
            renderPass_,
            0,
            0,
            0,
            nullptr);
    }

    RenderResponse Renderer::renderSelectedEntities(
        vk::Extent2D extent,
        const XMMATRIX & projView,
        const XMMATRIX & view,
        const std::shared_ptr<const std::vector<RenderEntity>> & entities,
        std::vector<uint32_t> selectedEntitiesIndex,
        uint32_t pipelineIndex,
        bool flipY,
        const vk::RenderPass & pass,
        int subPass,
        uint32_t pushOffset,
        uint32_t pushSize,
        void * pushData)
    {
        OPTICK_EVENT()

        auto iView = XMMatrixInverse(nullptr, view); // glm::inverse(view);
        auto right = XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(1, 0, 0, 0), iView));

        std::vector<std::tuple<MaterialPipelineId, MeshBundle *, uint32_t, uint32_t, MaterialId>>
            sorting_index;
        {
            OPTICK_EVENT("Build Sort Index")
            for (auto & eix: selectedEntitiesIndex) {
                const RenderEntity & entity = (*entities)[eix];

                auto pl = pipelineIndex == 1
                              ? entityManager_->getEntityOpaquePipeline(entity.entityId)
                              : entityManager_->getEntityShadowPipeline(entity.entityId);
                auto & e = entityManager_->getEntity(entity.entityId);

                uint8_t selected_lod = 0;

                if (e.lodCount > 1) {
                    auto s = getSphereSize(projView, right, entity.boundsSphere);
                    for (; selected_lod < e.lodCount && selected_lod < 4; selected_lod++) {
                        if (s > e.lods[selected_lod].second) {
                            break;
                        }
                    }
                    if (selected_lod == 4) {
                        selected_lod--;
                    }
                }

                auto mb = e.bundle;
                auto mix = e.lods[selected_lod];

                sorting_index.emplace_back(
                    std::tuple<MaterialPipelineId, MeshBundle *, uint32_t, uint32_t, MaterialId>{
                        pl,
                        mb.get(),
                        mix.first,
                        eix,
                        e.materialId
                    });
            }
        }
        {
            OPTICK_EVENT("Sort Entity Index")
            std::ranges::sort(
                sorting_index,
                [](const auto & a, const auto & b)
                {
                    auto & [apl, abund, amix, ax, amid] = a;
                    auto & [bpl, bbund, bmix, bx, bmid] = b;

                    if (apl < bpl) {
                        return true;
                    }
                    if (apl > bpl) {
                        return false;
                    }
                    if (abund < bbund) {
                        return true;
                    }
                    if (abund > bbund) {
                        return false;
                    }
                    return amix < bmix;
                }
            );
        }
        IndirectDrawSet ids;

        {
            OPTICK_EVENT("Build Draw Commands")
            MaterialPipelineId prevPL{};
            MeshBundle * prevBundle = nullptr;
            uint32_t prevMix = RX_INVALID_ID;

            uint32_t headerIndex = 0;
            uint32_t commandIndex = 0;

            for (auto & [pl, bundle, mix, eix, mid]: sorting_index) {
                if (prevPL != pl || bundle != prevBundle) {

                    headerIndex = static_cast<uint32_t>(ids.headers.size());
                    ids.headers
                        .push_back({
                            pl,
                            bundle,
                            static_cast<uint32_t>(ids.commands.size()),
                            0 });

                    prevPL = pl;
                    prevBundle = bundle;
                    prevMix = RX_INVALID_ID;
                }

                if (mix != prevMix) {
                    commandIndex = static_cast<uint32_t>(ids.commands.size());
                    auto mesh = bundle->getEntry(mix);
                    ids.commands.push_back({
                        mesh.indexCount,
                        mesh.vertexOffset,
                        mesh.indexOffset, 0, static_cast<uint32_t>(ids.instances.size()) }
                    );
                    ids.headers[headerIndex].commandCount++;
                    prevMix = mix;
                }
                auto & e = (*entities)[eix];

                ids.instances.push_back({ e.transform, e.instanceParams1, mid.index() , 0, 0, 0});
                ids.commands[commandIndex].instanceCount++;

                if (ids.instances.size() > 19990) {
                    spdlog::info("too many instances");
                    break;
                }
            }
        }

        if (ids.instances.empty()) {
            return {};
        }
        std::shared_ptr<RxCore::DescriptorSet> ds;
        std::shared_ptr<RxCore::Buffer> instanceBuffer;
        {
            OPTICK_EVENT("Get Buffer")
            std::lock_guard<std::mutex> lk(ibLock);

            ds = instanceBUfferDS[ibCycle];
            instanceBuffer = instanceBuffers[ibCycle];

            ibCycle = (ibCycle + 1) % 20;
        }
        auto ptr = instanceBuffer->getMemory()->getPtr();
        {
            OPTICK_EVENT("Copy to Instance Buffer")
            std::copy(ids.instances.begin(), ids.instances.end(),
                      reinterpret_cast<IndirectDrawInstance *>(ptr));
        }

        auto buf = RxCore::JobManager::threadData().getCommandBuffer();
        buf->begin(pass, subPass);
        OPTICK_GPU_CONTEXT(buf->Handle());

        buf->useLayout(pipelineLayout);
        if (pushSize > 0) {
            buf->pushConstant(vk::ShaderStageFlagBits::eVertex, pushOffset, pushSize, pushData);
        }

        buf->BindDescriptorSet(0, ds0_);
        buf->BindDescriptorSet(2, ds);

        setScissorAndViewport(extent, buf, flipY);

        {
            OPTICK_GPU_EVENT("ID Draws")
            renderIndirectDraws(ids, buf);
        }
        buf->end();
        return {buf};
    }

    RenderResponse Renderer::renderShadow(
        vk::Extent2D extent,
        const XMMATRIX & viewProj,
        const XMMATRIX & view,
        const BoundingOrientedBox & cullBox,
        uint32_t cascadeIndex,
        std::shared_ptr<const std::vector<RenderEntity>> entities)
    {
        OPTICK_EVENT()

        std::vector<uint32_t> visible_entity_index;
        std::vector<uint32_t> drawing_entity_index;

        cullEntitiesOrtho(cullBox, entities, visible_entity_index);

        MaterialPipelineId prev_pipeline;
        EntityId prev_entity_id;
        {
            OPTICK_EVENT("Select matching Pipeline")
            for (auto & i: visible_entity_index) {
                auto & e = (*entities)[i];
                MaterialPipelineId pl;
                if (e.entityId == prev_entity_id) {
                    pl = prev_pipeline;
                } else {
                    pl = entityManager_->getEntityShadowPipeline(e.entityId);
                    prev_pipeline = pl;
                    prev_entity_id = e.entityId;
                }
                if (pl.valid()) {
                    drawing_entity_index.insert(drawing_entity_index.end(), i);
                }
            }
        }

        if (drawing_entity_index.empty()) {
            return {};
        }

        return renderSelectedEntities(
            extent,
            viewProj,
            view,
            entities,
            drawing_entity_index,
            0,
            false,
            depthRenderPass_,
            0,
            0,
            sizeof(uint32_t),
            &cascadeIndex);
    }

    void Renderer::updateGui() const
    {
        lightingManager_->UpdateGui();
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
