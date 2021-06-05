#include <memory>
#include <algorithm>
#include <tuple>
#include "Renderer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/Queue.hpp"
#include <utility>
#include "Vulkan/FrameBuffer.h"
#include "robin_hood.h"
#include "DirectXCollision.h"
#include "EngineMain.hpp"
#include "Modules/Render.h"
#include "Vulkan/ThreadResources.h"
#include "Modules/SwapChain/SwapChain.h"

using namespace DirectX;

namespace RxEngine
{
    Renderer::Renderer(vk::Device device,
                       ecs::World * world,
                       vk::Format imageFormat,
                       EngineMain * engine,
                       const ecs::entity_t moduleId)
        : DeviceObject(device)
        , Module(world, engine, moduleId)
        , imageFormat_(imageFormat)
        , shadowImagesChanged(true)      
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

        descriptorPool = engine_->getDevice()->CreateDescriptorPool({
                {vk::DescriptorType::eCombinedImageSampler, 15000},
                {vk::DescriptorType::eUniformBufferDynamic, 600},
                {vk::DescriptorType::eStorageBuffer, 600},
                {vk::DescriptorType::eUniformBuffer, 800}
            },
            400);

        world_->setSingleton<RenderPasses>({
            renderPass_, 0,
            depthRenderPass_, 0,
            renderPass_, 0,
            renderPass_, 0
        });

        world_->addSingleton<FrameStats>();
        {
            auto fs = world_->getSingletonUpdate<FrameStats>();
            fs->frameNo = 0;
            fs->index = 0;
            fs->frames.resize(10);
        }
        //createPipelineLayout();

        //auto pl = world_->lookup("layout/general").get<PipelineLayout>();

        //pipelineLayout = pl->layout;
        //  dsLayouts.resize(pl->dsls.size());
        //dsLayouts[0] = pl->dsls[0];
        //dsLayouts[1] = pl->dsls[1];
        //dsLayouts[2] = pl->dsls[2];

        graphicsCommandPool_ = RxCore::Device::Context()->CreateGraphicsCommandPool();

        vk::QueryPoolCreateInfo qpci;

        qpci.queryType = vk::QueryType::eTimestamp;
        qpci.queryCount = 128;

        queryPool_ = RxCore::Device::VkDevice().createQueryPool(qpci);
#if 0
        for (uint32_t i = 0; i < 20; i++) {
            auto b = RxCore::iVulkan()->createBuffer(
                vk::BufferUsageFlagBits::eStorageBuffer,
                VMA_MEMORY_USAGE_CPU_TO_GPU,
                20000 * sizeof(IndirectDrawInstance)
            );

            b->getMemory()->map();
            auto ds = RxCore::threadResources.getDescriptorSet(
                poolTemplate, dsLayouts[2]);
            ds->updateDescriptor(0, vk::DescriptorType::eStorageBuffer, b);
            instanceBuffers.push_back(b);
            instanceBufferDS.push_back(ds);
        }
#endif
        world_->createSystem("Renderer:RunCommandBuffers")
              .inGroup("Pipeline:PostRender")
              //.after<AcquireImage>()
              //.before<PresentImage>()
              .withStream<MainRenderImageInput>()
              .withWrite<MainRenderImageOutput>()
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

        //auto q = world_->createQuery().with<DescriptorSet>().id;

        world_->createSystem("Renderer:CreateMainDescriptor")
              .withQuery<DescriptorSet>()
              .inGroup("Pipeline:PreFrame")
              .withRead<PipelineLayout>()
              .withWrite<DescriptorSet>()
              .withWrite<CurrentMainDescriptorSet>()
              .executeIfNone([this](ecs::World * world)
              {
                  auto pl = world_->lookup("layout/general").get<PipelineLayout>();
                  auto ds0_ = descriptorPool->allocateDescriptorSet(pl->dsls[0], { 1 });
//                  auto ds0_ = //RxCore::threadResources.getDescriptorSet(
  //                    poolTemplate,
    //                  pl->dsls[0], {1});

                  auto e = world->newEntity();
                  auto x = e.addAndUpdate<DescriptorSet>();
                  x->ds = ds0_;

                  world->setSingleton<CurrentMainDescriptorSet>({e.id});
              });
        //world_->addSingleton<Descriptors>();
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

    std::shared_ptr<const std::vector<RenderEntity>> Renderer::finishUpEntityJobs(
        const std::vector<std::shared_ptr<RxCore::Job<std::vector<RenderEntity>>>> & entityJobs)
    {
        OPTICK_EVENT()
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

        // std::vector<std::shared_ptr<RxCore::Job<RenderResponse>>> shadow_jobs;
        //std::vector<std::shared_ptr<RxCore::Job<RenderResponse>>> ui_jobs;
        //std::vector<std::shared_ptr<RxCore::Job<std::vector<RenderEntity>>>> entity_jobs;

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

        //std::vector<ShadowCascade> cascades;
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

                        world_->getStream<Render::OpaqueRenderCommand>()
                              ->each<Render::OpaqueRenderCommand>(
                                  [&](ecs::World * w, const Render::OpaqueRenderCommand * b)
                                  {
                                      buf->executeSecondary(b->buf);
                                      return true;
                                  }
                              );

                        world_->getStream<Render::UiRenderCommand>()
                              ->each<Render::UiRenderCommand>(
                                  [&](ecs::World * w, const Render::UiRenderCommand * b)
                                  {
                                      buf->executeSecondary(b->buf);
                                      return true;
                                  }
                              );
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
        auto fs = world_->getSingletonUpdate<FrameStats>();
        fs->frameNo++;
        fs->index = (fs->index + 1) % 10;
        fs->frames[fs->index].cpuTime = static_cast<float>(cpuTime);
    }
#if 0
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
#endif

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

    void Renderer::ensureShadowImages(uint32_t shadowMapSize, uint32_t numCascades)
    {
        if (shadowMap_ && shadowMap_->extent_.width == shadowMapSize) {
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

        cascadeViews_.resize(numCascades);
        cascadeFrameBuffers_.resize(numCascades);

        for (uint32_t i = 0; i < numCascades; ++i) {
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
            sci.setMagFilter(vk::Filter::eLinear)
               .setMinFilter(vk::Filter::eLinear)
               .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
               .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
               .setMaxLod(5.0f)
               .setBorderColor(vk::BorderColor::eFloatOpaqueWhite);

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
