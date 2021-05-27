//
// Created by shane on 19/02/2021.
//

#include <vector>
#include <memory>
#include <Loader.h>
#include <RXCore.h>
#include <AssetException.h>
#include "RmlRenderInterface.h"
#include "Log.h"
#include "Window.hpp"
#include "World.h"
#include "Modules/Render.h"
#include "Modules/Materials/Materials.h"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/ThreadResources.h"

using namespace DirectX;

namespace RxEngine
{
    void RmlRenderInterface::RenderGeometry(
        Rml::Vertex * vertices,
        int numVertices,
        int * indices,
        int numIndices,
        Rml::TextureHandle texture,
        const Rml::Vector2f & translation)
    {
        auto & r = renders.emplace_back();

        r.transform = transform_;
        r.texture = texture;

        size_t current_indices_size = indices_.size();
        r.indexOffset = current_indices_size;
        indices_.resize(numIndices + current_indices_size);
        r.indexCount = numIndices;

        memcpy(indices_.data() + current_indices_size, indices, sizeof(int) * numIndices);

        r.scissorEnable = scissorEnabled_;
        r.scissor = scissorRect;

        r.translation = XMFLOAT2(translation.x, translation.y);

        size_t current_vertex_size = vertices_.size();
        r.vertexOffset = current_vertex_size;

        vertices_.resize(numVertices + current_vertex_size);
        memcpy(
            vertices_.data() + current_vertex_size, // * sizeof(Rml::Vertex),
            vertices,
            sizeof(Rml::Vertex) * numVertices);
    }

    void RmlRenderInterface::EnableScissorRegion(bool enable)
    {
        scissorEnabled_ = enable;
    }

    void RmlRenderInterface::SetScissorRegion(int x, int y, int width, int height)
    {
        scissorRect = vk::Rect2D{
            vk::Offset2D{x, y},
            vk::Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)}
        };
    }

    RmlRenderInterface::RmlRenderInterface()
        : dirtyTextures(true)
        , transform_()
        , poolTemplate_({{vk::DescriptorType::eCombinedImageSampler, 5000},}, 10)
    {
        XMStoreFloat4x4(&transform_, XMMatrixIdentity());
    }

    bool RmlRenderInterface::LoadTexture(
        Rml::TextureHandle & texture_handle,
        Rml::Vector2i & texture_dimensions,
        const Rml::String & source)
    {
        OPTICK_EVENT()

        RxAssets::ImageData id{};
        try {
            RxAssets::Loader::loadImage(id, source);
        } catch (RxAssets::AssetException & e) {
            spdlog::error(e.what());
            return false;
        }

        auto image = RxCore::iVulkan()->createImage(
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
            auto staging_buffer = RxCore::iVulkan()->createStagingBuffer(
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

        //RXCore::Device::Context()->transitionImageLayout(
        //    image, vk::ImageLayout::eShaderReadOnlyOptimal);

        vk::SamplerCreateInfo sci{};
        sci.setMinFilter(vk::Filter::eNearest)
           .setMagFilter(vk::Filter::eNearest)
           .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
           .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
           .setMaxLod(1.0f)
           .setBorderColor(vk::BorderColor::eFloatOpaqueWhite);

        auto sampler = RxCore::iVulkan()->createSampler(sci);

        auto h = getNextTextureHandle();

        textureEntries_.emplace(h, RenderTextureEntry{image, iv, sampler});
        texture_handle = h;
        texture_dimensions.x = id.width;
        texture_dimensions.y = id.height;

        dirtyTextures = true;

        return true;
    }

    bool RmlRenderInterface::GenerateTexture(
        Rml::TextureHandle & texture_handle,
        const Rml::byte * source,
        const Rml::Vector2i & source_dimensions)
    {
        OPTICK_EVENT()
        auto image = RxCore::iVulkan()->createImage(
            vk::Format::eR8G8B8A8Unorm,
            vk::Extent3D{
                static_cast<uint32_t>(source_dimensions.x),
                static_cast<uint32_t>(source_dimensions.y), 1
            },
            1,
            1,
            vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
            vk::ImageType::e2D);

        auto iv =
            image->createImageView(vk::ImageViewType::e2D, vk::ImageAspectFlagBits::eColor);

        auto staging_buffer = RxCore::iVulkan()->createStagingBuffer(
            (source_dimensions.x * source_dimensions.y * 4), source);

        RxCore::iVulkan()->transferBufferToImage(
            staging_buffer,
            image,
            vk::Extent3D(static_cast<uint32_t>(source_dimensions.x),
                         static_cast<uint32_t>(source_dimensions.y), 1),
            vk::ImageLayout::eShaderReadOnlyOptimal,
            1,
            0);

        //RXCore::Device::Context()->transitionImageLayout(
        //    image, vk::ImageLayout::eShaderReadOnlyOptimal);

        vk::SamplerCreateInfo sci{};
        sci.setMinFilter(vk::Filter::eNearest)
           .setMagFilter(vk::Filter::eNearest)
           .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
           .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
           .setMaxLod(1.0f)
           .setBorderColor(vk::BorderColor::eFloatOpaqueWhite);

        auto sampler = RxCore::iVulkan()->createSampler(sci);

        auto h = getNextTextureHandle();

        texture_handle = h;

        textureEntries_.emplace(h, RenderTextureEntry{image, iv, sampler});
        dirtyTextures = true;

        return h;
    }

    void RmlRenderInterface::ReleaseTexture(Rml::TextureHandle texture)
    {
        OPTICK_EVENT()

        if (textureEntries_.contains(texture)) {
            textureEntries_.erase(texture);
        }

        dirtyTextures = true;
    }

    uintptr_t RmlRenderInterface::getNextTextureHandle() const
    {
        OPTICK_EVENT()

        uintptr_t h = 1;

        while (textureEntries_.contains(h)) {
            h++;
        }
        return h;
    }

    void RmlRenderInterface::SetTransform(const Rml::Matrix4f * transform)
    {
        if (transform == nullptr) {
            XMStoreFloat4x4(&transform_, XMMatrixIdentity());
        } else {
            memcpy(&transform_, transform, sizeof(transform_));
        }
    }

    void RmlRenderInterface::resetRender()
    {
        vertices_.clear();
        indices_.clear();
        renders.clear();
    }

    void RmlRenderInterface::renderUi(ecs::World * world)
    {
        if (!world->isAlive(pipeline_)) {
            pipeline_ = world->lookup("pipeline/rmlui");
        }
        auto wd = world->getSingleton<WindowDetails>();
        auto layout = pipeline_.getRelated<UsesLayout, PipelineLayout>();

        if (dirtyTextures) {
            auto descriptor_set = RxCore::threadResources.getDescriptorSet(
                poolTemplate_, layout->dsls[0], {
                    static_cast<uint32_t>(textureEntries_.size())
                });

            std::vector<RxCore::CombinedSampler> samplers;

            textureSamplerMap.clear();
            for (auto & te: textureEntries_) {
                auto ix = samplers.size();
                samplers.push_back({te.second.sampler, te.second.imageView});
                //sm.first = te.second.sampler;
                //sm.second = te.second.imageView;

                textureSamplerMap.emplace(te.first, static_cast<uint32_t>(ix));
            }

            if (!samplers.empty()) {
                descriptor_set->updateDescriptor(1, vk::DescriptorType::eCombinedImageSampler,
                                                 samplers);
            }

            auto pm = XMMatrixOrthographicOffCenterRH(
                0.0f,
                static_cast<float>(wd->width),
                0.0f,
                static_cast<float>(wd->height),
                0.0f,
                10.0f);

            XMStoreFloat4x4(&projectionMatrix_, pm);

            ub_ = RxCore::iVulkan()->createBuffer(
                vk::BufferUsageFlagBits::eUniformBuffer,
                VMA_MEMORY_USAGE_CPU_TO_GPU,
                sizeof(XMFLOAT4X4),
                &projectionMatrix_);
            ub_->map();
            descriptor_set->updateDescriptor(0, vk::DescriptorType::eUniformBuffer, ub_);

            currentDescriptorSet = descriptor_set;
            dirtyTextures = false;
        }
        auto pm = XMMatrixOrthographicOffCenterRH(
            0.0f,
            static_cast<float>(wd->width),
            0.0f,
            static_cast<float>(wd->height),
            0.0f,
            10.0f);

        XMStoreFloat4x4(&projectionMatrix_, pm);
        ub_->update(&projectionMatrix_, sizeof(XMFLOAT4X4));

        auto pipeline = pipeline_.get<GraphicsPipeline>();

        if (!pipeline) {
            return;
        }
        assert(pipeline);
        assert(pipeline->pipeline);

        auto buf = RxCore::threadResources.getCommandBuffer();

        if (vertices_.empty()) {
            return;
        }
        auto [vb, ib] = CreateBuffers();

        buf->begin(pipeline->renderPass, pipeline->subPass);
        OPTICK_GPU_CONTEXT(buf->Handle());
        {
            OPTICK_GPU_EVENT("Draw RlmUi");

            buf->useLayout(layout->layout);
            buf->bindPipeline(pipeline->pipeline->Handle());
            buf->BindDescriptorSet(0, currentDescriptorSet);

            buf->bindVertexBuffer(vb);
            buf->bindIndexBuffer(ib);
            buf->setViewport(0, 0, static_cast<float>(wd->width), static_cast<float>(wd->height), 0,
                             1);

            struct RmlPushConstantData pd{};

            for (auto & rc: renders) {
                pd.transform = rc.transform;
                pd.translate = rc.translation;
                if (rc.texture != 0) {
                    pd.textureId = textureSamplerMap[rc.texture];
                } else {
                    pd.textureId = 9999;
                }

                if (rc.scissorEnable) {
                    buf->setScissor(rc.scissor);
                } else {
                    buf->setScissor({{0, 0}, {wd->width, wd->height}});
                }
                buf->pushConstant(
                    vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                    0,
                    sizeof(RmlPushConstantData),
                    &pd);
                buf->DrawIndexed(
                    static_cast<uint32_t>(rc.indexCount),
                    1,
                    static_cast<uint32_t>(rc.indexOffset),
                    static_cast<uint32_t>(rc.vertexOffset),
                    0);
            }
        }
        buf->end();
        renders.clear();
        vertices_.clear();
        indices_.clear();

        world->getStream<Render::UiRenderCommand>()
             ->add<Render::UiRenderCommand>({buf});
    }

    RmlRenderInterface::~RmlRenderInterface()
    {
        currentDescriptorSet.reset();
    }

    std::tuple<std::shared_ptr<RxCore::VertexBuffer>, std::shared_ptr<RxCore::IndexBuffer>>
    RmlRenderInterface::CreateBuffers() const
    {
        auto vb =
            RxCore::iVulkan()->createVertexBuffer(
                VMA_MEMORY_USAGE_CPU_TO_GPU, static_cast<uint32_t>(vertices_.size()),
                static_cast<uint32_t>(sizeof(Rml::Vertex)));

        auto ib = RxCore::iVulkan()->createIndexBuffer(
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            static_cast<uint32_t>(indices_.size()), false);

        vb->map();
        ib->map();
        vb->update(vertices_.data(), vertices_.size() * static_cast<uint32_t>(sizeof(Rml::Vertex)));
        ib->update(indices_.data(), indices_.size() * static_cast<uint32_t>(sizeof(uint32_t)));
        vb->unmap();
        ib->unmap();

        return std::tuple(vb, ib);
    }
}
