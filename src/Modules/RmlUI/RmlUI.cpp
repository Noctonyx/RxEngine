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

#include <Modules/Materials/Materials.h>
#include "RmlUI.h"

#include "AssetException.h"
#include "EngineMain.hpp"
#include "Loader.h"
#include "Window.hpp"
#include "Modules/Render.h"
#include "optick/optick.h"
#include "RmlUi/Core/Context.h"
#include "RmlUi/Core/Core.h"
#include "RmlUi/Core/ElementDocument.h"
#include "RmlUi/Core/Input.h"
#include "RmlUi/Debugger/Debugger.h"
#include "RmlUi/Lua/Lua.h"
#include "spdlog/spdlog.h"
#include "Vulkan/ThreadResources.h"
#include "Vulkan/Image.hpp"

using namespace DirectX;

namespace RxEngine
{
    double RmlSystemInterface::GetElapsedTime()
    {
        return world_->getSingleton<EngineTime>()->totalElapsed;
        //return engine->getTotalElapsed();
    }

    bool RmlSystemInterface::LogMessage(Rml::Log::Type type, const Rml::String & message)
    {
        switch (type) {
        case Rml::Log::LT_ALWAYS:
            spdlog::critical(message);
            break;
        case Rml::Log::LT_DEBUG:
        case Rml::Log::LT_MAX:
            spdlog::debug(message);
            break;
        case Rml::Log::LT_ERROR:
        case Rml::Log::LT_ASSERT:
            spdlog::error(message);
            break;
        case Rml::Log::LT_INFO:
            spdlog::info(message);
            break;
        case Rml::Log::LT_WARNING:
            spdlog::warn(message);
            break;
        }
        return true;
    }

    RmlSystemInterface::RmlSystemInterface(ecs::World * world)
        : world_(world) {}

    int RmlSystemInterface::TranslateString(Rml::String & translated, const Rml::String & input)
    {
        return SystemInterface::TranslateString(translated, input);
    }

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
        scissorRect = VkRect2D{
            VkOffset2D{x, y},
            VkExtent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)}
        };
    }

    RmlRenderInterface::RmlRenderInterface(RxCore::Device * device)
        : device_(device)
        , dirtyTextures(true)
        , transform_()
    {
        XMStoreFloat4x4(&transform_, XMMatrixIdentity());
        descriptorPool = device->CreateDescriptorPool(
            {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5000}}, 5);
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

        auto image = device_->createImage(
            id.imType == RxAssets::eBC7
                ? VK_FORMAT_BC7_UNORM_BLOCK
                : VK_FORMAT_R8G8B8A8_UNORM,
            VkExtent3D{id.width, id.height, 1},
            static_cast<uint32_t>(id.mipLevels.size()),
            1,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_TYPE_2D);

        auto iv =
            device_->createImageView(image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT);

        for (uint32_t j = 0; j < id.mipLevels.size(); j++) {
            auto staging_buffer = device_->createStagingBuffer(
                id.mipLevels[j].bytes.size(), id.mipLevels[j].bytes.data());

            device_->transferBufferToImage(
                staging_buffer,
                image,
                VkExtent3D{ id.mipLevels[j].width, id.mipLevels[j].height, 1 },
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                1,
                0,
                j);
        }

        //RXCore::Device::Context()->transitionImageLayout(
        //    image, VkImageLayout::eShaderReadOnlyOptimal);

        VkSamplerCreateInfo sci{};
        sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        sci.minFilter = (VK_FILTER_NEAREST);
        sci.magFilter = VK_FILTER_NEAREST;
        sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sci.maxLod = 1.0f;
        sci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

        auto sampler = device_->createSampler(sci);

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
        auto image = device_->createImage(
            VK_FORMAT_R8G8B8A8_UNORM,
            VkExtent3D{
                static_cast<uint32_t>(source_dimensions.x),
                static_cast<uint32_t>(source_dimensions.y), 1
            },
            1,
            1,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_IMAGE_TYPE_2D);

        auto iv =
            device_->createImageView(image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT);

        auto staging_buffer = device_->createStagingBuffer(
            (source_dimensions.x * source_dimensions.y * 4), source);

        device_->transferBufferToImage(
            staging_buffer,
            image,
            VkExtent3D{ static_cast<uint32_t>(source_dimensions.x),
                         static_cast<uint32_t>(source_dimensions.y), 1 },
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            1,
            0);

        //RXCore::Device::Context()->transitionImageLayout(
        //    image, VkImageLayout::eShaderReadOnlyOptimal);

        VkSamplerCreateInfo sci{};
        sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        sci.minFilter = (VK_FILTER_NEAREST);
        sci.magFilter = VK_FILTER_NEAREST;
        sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sci.maxLod = 1.0f;
        sci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

        auto sampler = device_->createSampler(sci);

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
            auto descriptor_set = descriptorPool->allocateDescriptorSet(
                layout->dsls[0], {static_cast<uint32_t>(textureEntries_.size())});
            //            auto descriptor_set = RxCore::threadResources.getDescriptorSet(
            //              poolTemplate_, layout->dsls[0], {
            //                static_cast<uint32_t>(textureEntries_.size())
            //          });

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
                descriptor_set->updateDescriptor(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
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

            ub_ = device_->createBuffer(
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU,
                sizeof(XMFLOAT4X4),
                &projectionMatrix_);
            ub_->map();
            descriptor_set->updateDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, ub_);

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
        uint32_t triangles = 0;
        uint32_t drawCalls = 0;

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
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    sizeof(RmlPushConstantData),
                    &pd);
                drawCalls++;
                triangles += static_cast<uint32_t>(rc.indexCount) / 3;
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

        world->getStream<Render::GameUiRenderCommand>()
             ->add<Render::GameUiRenderCommand>({buf, triangles, drawCalls});
    }

    RmlRenderInterface::~RmlRenderInterface()
    {
        currentDescriptorSet.reset();
    }

    std::tuple<std::shared_ptr<RxCore::VertexBuffer>, std::shared_ptr<RxCore::IndexBuffer>>
    RmlRenderInterface::CreateBuffers() const
    {
        auto vb =
           device_->createVertexBuffer(
                VMA_MEMORY_USAGE_CPU_TO_GPU, static_cast<uint32_t>(vertices_.size()),
                static_cast<uint32_t>(sizeof(Rml::Vertex)));

        auto ib = device_->createIndexBuffer(
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

    Rml::FileHandle RmlFileInterface::Open(const Rml::String & path)
    {
        auto size = vfs_->getFilesize(path);
        if (!size.has_value()) {
            return 0;
        }

        auto handle = nextHandle++;

        auto [iterator, r] = fileData.emplace(handle, FileHandle{});
        assert(r);

        iterator->second.size = size.value();
        iterator->second.pointer = 0;
        iterator->second.buffer.resize(size.value());
        vfs_->getFileContents(path, iterator->second.buffer.data());

        return handle;
    }

    void RmlFileInterface::Close(Rml::FileHandle file)
    {
        fileData.erase(file);
    }

    size_t RmlFileInterface::Read(void * buffer, size_t size, Rml::FileHandle file)
    {
        assert(fileData.contains(file));

        auto & fd = fileData[file];

        if (!(fd.pointer + size <= fd.size)) {
            size = fd.size - fd.pointer;
        }

        memcpy(buffer, fd.buffer.data() + fd.pointer, size);
        fd.pointer += size;

        return size;
    }

    bool RmlFileInterface::Seek(Rml::FileHandle file, long offset, int origin)
    {
        assert(fileData.contains(file));

        auto & fd = fileData[file];

        switch (origin) {
        case SEEK_CUR:
            if (offset <= static_cast<long>(fd.size)) {
                fd.pointer = offset;
                return true;
            }
            return false;
        case SEEK_END:
            fd.pointer = fd.size - offset;
            return true;
        case SEEK_SET:
            if (offset <= static_cast<long>(fd.size)) {
                fd.pointer = offset;
                return true;
            }
            return false;
        default:
            return false;
        }
    }

    size_t RmlFileInterface::Tell(Rml::FileHandle file)
    {
        assert(fileData.contains(file));

        auto & fd = fileData[file];

        return fd.pointer;
    }

    RmlFileInterface::RmlFileInterface()
    {
        vfs_ = RxAssets::Vfs::getInstance();
    }


    int convertModState(RxEngine::EInputMod mods)
    {
        int kms = 0;
        if (static_cast<uint32_t>(mods) & static_cast<uint32_t>(EInputMod_Shift)) {
            kms |= Rml::Input::KeyModifier::KM_SHIFT;
        }
        if (static_cast<uint32_t>(mods) & static_cast<uint32_t>(EInputMod_Control)) {
            kms |= Rml::Input::KeyModifier::KM_CTRL;
        }
        if (static_cast<uint32_t>(mods) & static_cast<uint32_t>(EInputMod_Alt)) {
            kms |= Rml::Input::KeyModifier::KM_ALT;
        }
        return kms;
    }

    Rml::Input::KeyIdentifier convertKey(EKey key)
    {
        switch (key) {
        case EKey::Space:
            return Rml::Input::KeyIdentifier::KI_SPACE;
        case EKey::Apostrophe:
            return Rml::Input::KeyIdentifier::KI_OEM_3;
        case EKey::Comma:
            return Rml::Input::KeyIdentifier::KI_OEM_COMMA;
        case EKey::F1:
            return Rml::Input::KeyIdentifier::KI_F1;
        case EKey::F2:
            return Rml::Input::KeyIdentifier::KI_F2;
        case EKey::F3:
            return Rml::Input::KeyIdentifier::KI_F3;
        case EKey::F4:
            return Rml::Input::KeyIdentifier::KI_F4;
        case EKey::F5:
            return Rml::Input::KeyIdentifier::KI_F5;
        case EKey::F6:
            return Rml::Input::KeyIdentifier::KI_F6;
        case EKey::F7:
            return Rml::Input::KeyIdentifier::KI_F7;
        case EKey::F8:
            return Rml::Input::KeyIdentifier::KI_F8;
        case EKey::F9:
            return Rml::Input::KeyIdentifier::KI_F9;
        case EKey::F10:
            return Rml::Input::KeyIdentifier::KI_F10;

        default:
            return Rml::Input::KeyIdentifier::KI_UNKNOWN;
        }
    }
#if 0
    Rml::ElementDocument * UiContext::loadDocument(const std::string & document)
    {
        auto doc = context->LoadDocument(document);
        documents[document] = doc;
        doc->Show();

        return doc;
    }

    void UiContext::closeDocument(const std::string & document)
    {
        auto it = documents.find(document);
        if(it == documents.end()) {
            return;
        }
        it->second->Close();
        documents.erase(it);
    }
#endif
    void RmlUiModule::startup()
    {
        rmlSystem = std::make_unique<RmlSystemInterface>(world_);
        rmlFile = std::make_unique<RmlFileInterface>();
        rmlRender = std::make_unique<RmlRenderInterface>(engine_->getDevice());

        Rml::SetSystemInterface(rmlSystem.get());
        Rml::SetFileInterface(rmlFile.get());
        Rml::SetRenderInterface(rmlRender.get());
        Rml::Initialise();

        Rml::LoadFontFace("/ui/fonts/TitilliumWeb-Regular.ttf");
        Rml::LoadFontFace("/ui/LatoLatin-Regular.ttf");
        Rml::LoadFontFace("/ui/fonts/Roboto-Regular.ttf");
        Rml::LoadFontFace("/ui/fonts/Roboto-Bold.ttf");

        Rml::Lua::Initialise(engine_->getLua()->lua_state());

        world_->createSystem("RmlUI:Resize")
              .inGroup("Pipeline:Update")
              .withStream<WindowResize>()
              .execute<WindowResize>(
                  [this](ecs::World *, const WindowResize * resize)
                  {
                      mainUI->SetDimensions(
                          Rml::Vector2i{
                              static_cast<int>(resize->width),
                              static_cast<int>(resize->height)
                          });
                      rmlRender->setDirty();
                      return false;
                  }
              );
#if 0
        world_->createSystem("RmlUI:ResizeContexts")
              .withQuery<UiContext, UiContextCreated>()
              .inGroup("Pipeline:Early")
              .withWrite<WindowResize>()
              .each<UiContext>([&](ecs::EntityHandle e, const UiContext * ctx)
              {
                  OPTICK_EVENT()
                  e.world->getStream<WindowResize>()->each<WindowResize>(
                      [&](ecs::World *, const WindowResize * resize)
                      {
                          ctx->context->SetDimensions(
                              Rml::Vector2i{
                                  static_cast<int>(resize->width),
                                  static_cast<int>(resize->height)
                              });
                          return false;
                      });
              });
#endif
        world_->createSystem("Rml::MousePosition")
              .inGroup("Pipeline:Early")
              .withStream<MousePosition>()
              .execute<MousePosition>([this](ecs::World *, const MousePosition * mp)
              {
                  if (mp->captured) {
                      return false;
                  }

                  return !mainUI->ProcessMouseMove(static_cast<int>(mp->x),
                                                   static_cast<int>(mp->y),
                                                   convertModState(mp->mods));
              });

        world_->createSystem("Rml::MouseButton")
              .inGroup("Pipeline:Early")
              .withStream<MouseButton>()
              .execute<MouseButton>([this](ecs::World *, const MouseButton * button)
              {
                  if (button->pressed) {
                      return !mainUI->ProcessMouseButtonDown(
                          button->button, convertModState(button->mods));
                  } else {
                      return !mainUI->ProcessMouseButtonUp(
                          button->button, convertModState(button->mods));
                  }
              });


        world_->createSystem("Rml::MouseScroll")
              .inGroup("Pipeline:Early")
              .withStream<MouseScroll>()
              .execute<MouseScroll>([this](ecs::World *, const MouseScroll * s)
              {
                  return !mainUI->ProcessMouseWheel(
                      -s->y_offset, convertModState(s->mods));
              });

#if 0
        world_->createSystem("RmlUI:Mouse")
              .withQuery<UiContext, UiContextCreated, UiContextInteractive>()
              .inGroup("Pipeline:Early")
              .withWrite<MousePosition>()
              .withWrite<MouseButton>()
              .withWrite<MouseScroll>()
              .each<UiContext>([&](ecs::EntityHandle e, const UiContext * ctx)
              {
                  OPTICK_EVENT()
                  e.world->getStream<MousePosition>()->each<MousePosition>(
                      [&](ecs::World *, const MousePosition * mp)
                      {
                          return !ctx->context->ProcessMouseMove(static_cast<int>(mp->x),
                                                                 static_cast<int>(mp->y),
                                                                 convertModState(mp->mods));
                      });

                  e.world->getStream<MouseButton>()->each<MouseButton>(
                      [&](ecs::World *, const MouseButton * button)
                      {
                          if (button->pressed) {
                              return !ctx->context->ProcessMouseButtonDown(
                                  button->button, convertModState(button->mods));
                          } else {
                              return !ctx->context->ProcessMouseButtonUp(
                                  button->button, convertModState(button->mods));
                          }
                      });

                  e.world->getStream<MouseScroll>()->each<MouseScroll>(
                      [&](ecs::World *, const MouseScroll * s)
                      {
                          return !ctx->context->ProcessMouseWheel(
                              -s->y_offset, convertModState(s->mods));
                      });
              });

#endif
        world_->createSystem("Rml::KeyboardKey")
              .inGroup("Pipeline:Early")
              .withStream<KeyboardKey>()
              .execute<KeyboardKey>([this](ecs::World *, const KeyboardKey * key)
              {
                  if (key->key == EKey::F8 && key->action == EInputAction::Press) {
                      Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
                  }
                  if (key->key == EKey::F9 && key->action == EInputAction::Press) {
                      for (int i = 0; i < mainUI->GetNumDocuments(); i++) {
                          auto doc = mainUI->GetDocument(i);
                          if (doc->GetSourceURL() != "") {
                              doc->ReloadStyleSheet();
                          }
                      }
                  }
                  if (key->key == EKey::F10 && key->action == EInputAction::Press) {
                      std::vector<Rml::String> urls;
                      for (int i = 0; i < mainUI->GetNumDocuments(); i++) {
                          auto doc = mainUI->GetDocument(i);
                          if (doc->GetSourceURL() != "") {
                              urls.push_back(doc->GetSourceURL());
                              doc->Close();
                          }
                      }
                      for (auto & u: urls) {
                          mainUI->LoadDocument(u)->Show();
                      }
                  }
                  if (key->action == EInputAction::Press) {
                      return !mainUI->ProcessKeyDown(convertKey(key->key),
                                                     convertModState(key->mods));
                  }
                  if (key->action == EInputAction::Release) {
                      return !mainUI->ProcessKeyUp(convertKey(key->key),
                                                   convertModState(key->mods));
                  }

                  return false;
              });

        world_->createSystem("Rml::KeyboardChar")
              .inGroup("Pipeline:Early")
              .withStream<KeyboardChar>()
              .execute<KeyboardChar>([this](ecs::World *, const KeyboardChar * c)
              {
                  return mainUI->ProcessTextInput(c->c);
              });

#if 0
        world_->createSystem("Rml:RenderContext")
              .inGroup("Pipeline:PreRender")
              .withQuery<UiContext, UiContextCreated>()
              .withWrite<UiContextProcessed>()
              .each<UiContext>([&](ecs::EntityHandle e, const UiContext * ctx)
              {
                  OPTICK_EVENT()
                  ctx->context->Update();
                  ctx->context->Render();
              });
#endif
        world_->createSystem("Rml:Render")
              .inGroup("Pipeline:PreRender")
              //.withRead<UiContextProcessed>()
              .withStreamWrite<Render::GameUiRenderCommand>()
              .withJob()
              .execute([this](ecs::World *)
              {
                  OPTICK_EVENT("Rml:Render")
                  mainUI->Update();
                  mainUI->Render();
                  rmlRender->renderUi(world_);
              });

        auto wd = world_->getSingleton<WindowDetails>();

        mainUI = Rml::CreateContext(
            "MainUI",
            Rml::Vector2i(static_cast<int>(wd->width), static_cast<int>(wd->height)));
        Rml::Debugger::Initialise(mainUI);
    }

    void RmlUiModule::shutdown()
    {
        world_->deleteSystem(world_->lookup("RmlUI:Resize"));
        world_->deleteSystem(world_->lookup("RmlUI:Render"));
        //        world_->deleteSystem(world_->lookup("RmlUI:RenderContext"));
        world_->deleteSystem(world_->lookup("RmlUI:KeyboardKey"));
        world_->deleteSystem(world_->lookup("RmlUI:KeyboardChar"));
        world_->deleteSystem(world_->lookup("RmlUI:MousePosition"));
        world_->deleteSystem(world_->lookup("RmlUI:MouseButton"));
        world_->deleteSystem(world_->lookup("RmlUI:MouseScroll"));
        //world_->deleteSystem(world_->lookup("RmlUI:NewContext"));

        Rml::RemoveContext("MainUI");
        Rml::Shutdown();

        rmlRender.reset();
        rmlSystem.reset();
        rmlFile.reset();
    }

    void RmlUiModule::addLuaEnvironment(sol::state & state)
    {
        Rml::Lua::Initialise(state.lua_state());
    }
}
