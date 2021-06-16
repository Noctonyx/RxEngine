////////////////////////////////////////////////////////////////////////////////
// MIT License
//
// Copyright (c) 2021-2021.  Shane Hyde (shane@noctonyx.com)
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
#include "Modules/SceneCamera/SceneCamera.h"
#include "Vulkan/ThreadResources.h"
#include "Modules/SwapChain/SwapChain.h"

using namespace DirectX;

namespace RxEngine
{
    Renderer::Renderer(RxCore::Device * device,
                       ecs::World * world,
                       EngineMain * engine,
                       const ecs::entity_t moduleId)
        : Module(world, engine, moduleId)
        , device_(device)
        // , imageFormat_(imageFormat)
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

        descriptorPool = engine_->getDevice()->CreateDescriptorPool(
            {
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 15000},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 600},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 600},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 800}
            },
            400
        );

        world_->setSingleton<RenderPasses>(
            {
                renderPass_, 0,
                depthRenderPass_, 0,
                renderPass_, 0,
                renderPass_, 0
            }
        );

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

        graphicsCommandPool_ = device_->CreateGraphicsCommandPool();

        VkQueryPoolCreateInfo qpci{};

        qpci.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        qpci.queryType = VK_QUERY_TYPE_TIMESTAMP;
        qpci.queryCount = 128;

        vkCreateQueryPool(device_->getDevice(), &qpci, nullptr, &queryPool_);
        //        queryPool_ = device_->getDevice().createQueryPool(qpci);
#if 0
        for (uint32_t i = 0; i < 20; i++) {
            auto b = RxCore::iVulkan()->createBuffer(
                VkBufferUsageFlagBits::eStorageBuffer,
                VMA_MEMORY_USAGE_CPU_TO_GPU,
                20000 * sizeof(IndirectDrawInstance)
            );

            b->getMemory()->map();
            auto ds = RxCore::threadResources.getDescriptorSet(
                poolTemplate, dsLayouts[2]);
            ds->updateDescriptor(0, VkDescriptorType::eStorageBuffer, b);
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
                      render(
                          mri->imageView,
                          mri->extent,
                          {mri->imageAvailableSempahore},
                          {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
                          mri->finishRenderSemaphore
                      );

                      world->getStream<MainRenderImageOutput>()
                           ->add<MainRenderImageOutput>(
                               {mri->imageView, mri->finishRenderSemaphore}
                           );

                      return true;
                  }
              );

        world_->createSystem("Renderer:CleanLastFrame")
              .inGroup("Pipeline:PreRender")
              .execute(
                  [this](ecs::World *)
                  {
                      OPTICK_EVENT("Release Previous Frame Resource")
                      device_->graphicsQueue_->ReleaseCompleted();
                  }
              );

        //auto q = world_->createQuery().with<DescriptorSet>().id;

        world_->createSystem("Renderer:CreateMainDescriptor")
              .withQuery<DescriptorSet>()
              .inGroup("Pipeline:PreFrame")
              .withRead<PipelineLayout>()
              .withWrite<DescriptorSet>()
              .withWrite<CurrentMainDescriptorSet>()
              .executeIfNone(
                  [this](ecs::World * world)
                  {
                      auto pl = world_->lookup("layout/general").get<PipelineLayout>();
                      auto ds0x_ = descriptorPool->allocateDescriptorSet(pl->dsls[0], pl->counts);
                      //                  auto ds0_ = //RxCore::threadResources.getDescriptorSet(
                      //                    poolTemplate,
                      //                  pl->dsls[0], {1});

                      auto e = world->newEntity();
                      auto x = e.addAndUpdate<DescriptorSet>();
                      x->ds = ds0x_;

                      world->setSingleton<CurrentMainDescriptorSet>({e.id});
                  }
              );

        //world_->addSingleton<Descriptors>();
    }

    void Renderer::createDepthRenderPass()
    {
        std::vector<VkAttachmentDescription> ad = {
            {
                {},
                device_->GetDepthFormat(true),

                VK_SAMPLE_COUNT_1_BIT,
                VK_ATTACHMENT_LOAD_OP_CLEAR,
                VK_ATTACHMENT_STORE_OP_STORE,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
            }
        };

        std::vector<VkAttachmentReference> input_attachments = {};
        std::vector<VkAttachmentReference> color_attachments = {};
        std::vector<VkAttachmentReference> resolve_attachments = {};
        VkAttachmentReference depth_attachment = {
            0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        };
        std::vector<uint32_t> preserve_attachments = {};

        std::vector<VkSubpassDescription> sp = {
            {
                0,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                static_cast<uint32_t>(input_attachments.size()),
                input_attachments.data(),
                static_cast<uint32_t>(color_attachments.size()),
                color_attachments.data(),
                resolve_attachments.data(),
                &depth_attachment,
                0,
                nullptr
            }
        };

        std::vector<VkSubpassDependency> spd = {
            {
                VK_SUBPASS_EXTERNAL,
                0,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_DEPENDENCY_BY_REGION_BIT
            },
            {
                0,
                VK_SUBPASS_EXTERNAL,
                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                VK_DEPENDENCY_BY_REGION_BIT
            }
        };

        VkRenderPassCreateInfo rpci{};
        rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpci.attachmentCount = static_cast<uint32_t>(ad.size());
        rpci.pAttachments = ad.data();
        rpci.subpassCount = static_cast<uint32_t>(sp.size());
        rpci.pSubpasses = sp.data();
        rpci.dependencyCount = static_cast<uint32_t>(spd.size());
        rpci.pDependencies = spd.data();
        //rpci.setAttachments(ad).setSubpasses(sp).setDependencies(spd);

        vkCreateRenderPass(device_->getDevice(), &rpci, nullptr, &depthRenderPass_);
        //auto rph = device_->getDevice().createRenderPass(rpci);
        //depthRenderPass_ = rph;
    }

    void Renderer::createRenderPass()
    {
        auto imageFormat = device_->getSwapChainFormat();

        std::vector<VkAttachmentDescription> ads{
            {
                {},
                imageFormat,
                VK_SAMPLE_COUNT_1_BIT,
                VK_ATTACHMENT_LOAD_OP_CLEAR,
                VK_ATTACHMENT_STORE_OP_STORE,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR // ! important
            },
            {
                {},
                device_->GetDepthFormat(false),
                VK_SAMPLE_COUNT_1_BIT,
                VK_ATTACHMENT_LOAD_OP_CLEAR,
                VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            }
        };

        std::vector<VkAttachmentReference> input_attachments = {};
        std::vector<VkAttachmentReference> color_attachments{
            {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}
        };
        std::vector<VkAttachmentReference> resolve_attachments = {};
        VkAttachmentReference depth_attachment{
            1,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        };
        std::vector<uint32_t> preserve_attachments = {};

        std::vector<VkSubpassDescription> sp{
            {
                {},
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                static_cast<uint32_t>(input_attachments.size()),
                input_attachments.data(),
                static_cast<uint32_t>(color_attachments.size()),
                color_attachments.data(),
                resolve_attachments.data(),
                &depth_attachment,
                0, nullptr
            }
        };

        std::vector<VkSubpassDependency> spd = {
            {
                VK_SUBPASS_EXTERNAL,
                0,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                {},
                VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_DEPENDENCY_BY_REGION_BIT
            }
        };
        VkRenderPassCreateInfo rpci{};
        rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpci.attachmentCount = static_cast<uint32_t>(ads.size());
        rpci.pAttachments = ads.data();
        rpci.subpassCount = static_cast<uint32_t>(sp.size());
        rpci.pSubpasses = sp.data();
        rpci.dependencyCount = static_cast<uint32_t>(spd.size());
        rpci.pDependencies = spd.data();

        vkCreateRenderPass(device_->getDevice(), &rpci, nullptr, &renderPass_);
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
        VkImageView imageView,
        VkExtent2D extent,
        std::vector<VkSemaphore> waitSemaphores,
        std::vector<VkPipelineStageFlags> waitStages,
        VkSemaphore completeSemaphore)
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

        uint32_t total_triangles = 0;
        uint32_t total_draws = 0;

        std::shared_ptr<RxCore::FrameBuffer> frame_buffer;
        frame_buffer = createRenderFrameBuffer(imageView, extent);
        {
            OPTICK_EVENT("Build Primary Buffer")
            buf->begin();
            OPTICK_GPU_CONTEXT(buf->Handle())
            {
                vkCmdResetQueryPool(buf->Handle(), queryPool_, 0, 128);
                //buf->Handle().resetQueryPool(queryPool_, 0, 128);
                vkCmdWriteTimestamp(buf->Handle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, queryPool_,
                                    0);
                //                buf->Handle().writeTimestamp(VkPipelineStageFlagBits::eTopOfPipe, queryPool_, 0);

                std::vector<VkClearValue> depth_clear_values = {VkClearValue({{{1.0f, ~0u}}})};

                {
                    OPTICK_GPU_EVENT("Shadow RenderPass")
                    for (uint32_t i = 0; i < NUM_CASCADES; i++) {
                        buf->beginRenderPass(
                            depthRenderPass_, cascadeFrameBuffers_[i],
                            VkExtent2D{4096, 4096}, depth_clear_values
                        );
                        {
                            OPTICK_EVENT("Execute Secondaries")

                            buf->executeSecondaries(static_cast<uint16_t>(1000 + i));
                        }
                        buf->EndRenderPass();
                    }
                }
                std::vector<VkClearValue> clear_values = {
                    {{{0.0f, 0.0f, 0.0f, 1.0f}}},
                    {{{1.0f, ~0u}}}
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
                                      total_draws += b->drawCalls;
                                      total_triangles += b->triangles;
                                      buf->executeSecondary(b->buf);
                                      return true;
                                  }
                              );

                        world_->getStream<Render::GameUiRenderCommand>()
                              ->each<Render::GameUiRenderCommand>(
                                  [&](ecs::World * w, const Render::GameUiRenderCommand * b)
                                  {
                                      total_draws += b->drawCalls;
                                      total_triangles += b->triangles;
                                      buf->executeSecondary(b->buf);
                                      return true;
                                  }
                              );
                        world_->getStream<Render::EngineUiRenderCommand>()
                              ->each<Render::EngineUiRenderCommand>(
                                  [&](ecs::World * w, const Render::EngineUiRenderCommand * b)
                                  {
                                      total_draws += b->drawCalls;
                                      total_triangles += b->triangles;
                                      buf->executeSecondary(b->buf);
                                      return true;
                                  }
                              );
                    }
                    buf->EndRenderPass();
                }
                vkCmdWriteTimestamp(buf->Handle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool_,
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
            device_->graphicsQueue_->Submit(
                {buf}, std::move(waitSemaphores), std::move(waitStages), {completeSemaphore}
            );
        }

        const auto end_time = std::chrono::high_resolution_clock::now();
        cpuTime = std::chrono::duration<double, std::milli>((end_time - start_time)).count();
        auto fs = world_->getSingletonUpdate<FrameStats>();
        fs->frameNo++;
        fs->index = (fs->index + 1) % 10;
        fs->frames[fs->index].cpuTime = static_cast<float>(cpuTime);
        fs->frames[fs->index].drawCalls = total_draws;
        fs->frames[fs->index].triangles = total_triangles;
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
                VkDescriptorType::eCombinedImageSampler,
                wholeShadowMapView_,
                VkImageLayout::eDepthStencilReadOnlyOptimal,
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
        const VkImageView & imageView,
        const VkExtent2D & extent) const
    {
        OPTICK_EVENT("Create Framebuffer")
        std::vector<VkImageView> attachments = {imageView, depthBufferView_->handle_};

        VkFramebufferCreateInfo fbci{};
        fbci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbci.renderPass = renderPass_;
        fbci.attachmentCount = static_cast<uint32_t>(attachments.size());
        fbci.pAttachments = attachments.data();
        fbci.width = extent.width;
        fbci.height = extent.height;
        fbci.layers = 1;

        VkFramebuffer fb;

        vkCreateFramebuffer(device_->getDevice(), &fbci, nullptr, &fb);

        auto frame_buffer = std::make_shared<RxCore::FrameBuffer>(device_, fb);

        return frame_buffer;
    }

#if 0
    void Renderer::ensureFrameBufferSize(const VkImageView & imageView, const VkExtent2D & extent)
    {
        if (frameBuffers_[frameBufferIndex_].extent != extent) {

            std::vector<VkImageView> attachments = {
                imageView,
                depthBufferView_->handle
            };
            //auto fbh = renderPass->CreateFrameBuffer(attachments, extent.width, extent.height);

            VkFramebufferCreateInfo fbci{
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

    void Renderer::ensureDepthBufferExists(VkExtent2D & extent)
    {
        if (extent.height != bufferExtent_.height || extent.width != bufferExtent_.width) {
            depthBuffer_ = device_->createImage(
                device_->GetDepthFormat(false),
                {extent.width, extent.height, 1},
                1, 1,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
            );

            depthBufferView_ = device_->createImageView(
                depthBuffer_,
                VK_IMAGE_VIEW_TYPE_2D,
                VK_IMAGE_ASPECT_DEPTH_BIT,
                0,
                1
            );
            bufferExtent_ = extent;
        }
    }

    void Renderer::shutdown()
    {
        world_->deleteSystem(world_->lookup("Renderer:Render").id);

        engine_->getDevice()->WaitIdle();
        vkDestroyQueryPool(device_->getDevice(), queryPool_, nullptr);
        //device_->getDevice().destroyQueryPool(queryPool_);
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

        vkDestroyRenderPass(device_->getDevice(), renderPass_, nullptr);
        vkDestroyRenderPass(device_->getDevice(), depthRenderPass_, nullptr);
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
        auto device = engine_->getDevice();
        shadowImagesChanged = true;
        shadowMap_ = device->createImage(
            device->GetDepthFormat(false),
            {shadowMapSize, shadowMapSize, 1},
            1,
            numCascades,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        );

        wholeShadowMapView_ = device->createImageView(
            shadowMap_,
            VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_ASPECT_DEPTH_BIT, 0, numCascades
        );

        cascadeViews_.
            resize(numCascades);
        cascadeFrameBuffers_.
            resize(numCascades);

        for (uint32_t i = 0; i < numCascades; ++i) {
            cascadeViews_[i] = device->createImageView(
                shadowMap_,
                VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                VK_IMAGE_ASPECT_DEPTH_BIT, i,
                1);

            std::vector<VkImageView> attachments = {cascadeViews_[i]->handle_};

            VkFramebufferCreateInfo fbci{};
            fbci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbci.renderPass = depthRenderPass_;
            fbci.attachmentCount = static_cast<uint32_t>(attachments.size());
            fbci.pAttachments = attachments.data();
            fbci.width = shadowMapSize;
            fbci.height = shadowMapSize;
            fbci.layers = 1;

            VkFramebuffer fb;

            vkCreateFramebuffer(device_->getDevice(), &fbci, nullptr, &fb);

            cascadeFrameBuffers_[i] = std::make_shared<RxCore::FrameBuffer>(device_, fb);
        }

        if (!shadowSampler_) {
            VkSamplerCreateInfo sci{};
            sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            sci.magFilter = VK_FILTER_LINEAR;
            sci.minFilter = VK_FILTER_LINEAR;
            sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sci.maxLod = 0.5f;
            sci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

            shadowSampler_ = device->createSampler(sci);
        }
    }

    void Renderer::setScissorAndViewport(
        VkExtent2D extent,
        std::shared_ptr<RxCore::SecondaryCommandBuffer> buf,
        bool flipY) const
    {
        buf->setScissor(
            {
                {0, 0},
                {extent.width, extent.height}
            }
        );
        buf->setViewport(
            .0f, flipY ? static_cast<float>(extent.height) : 0.0f, static_cast<float>(extent.width),
            flipY ? -static_cast<float>(extent.height) : static_cast<float>(extent.height), 0.0f,
            1.0f
        );
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
