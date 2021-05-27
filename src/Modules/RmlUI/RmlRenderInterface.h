//
// Created by shane on 19/02/2021.
//

#ifndef RX_RMLRENDERINTERFACE_H
#define RX_RMLRENDERINTERFACE_H

#include <unordered_map>
#include <vector>
#include "DirectXMath.h"
#include "Vulkan/Vulk.hpp"
#include <RmlUi/Core/RenderInterface.h>
#include "EntityHandle.h"
#include "Vulkan/DescriptorPool.hpp"

namespace ecs {
    class World;
}

namespace RxCore
{
    class Image;
    class ImageView;
    class Buffer;
    class VertexBuffer;
    class IndexBuffer;
    struct DescriptorPoolTemplate;
}

namespace RxEngine
{
    class EngineMain;

    struct UiRenderEntry
    {
        size_t vertexOffset;
        size_t indexOffset;
        size_t indexCount;

        Rml::TextureHandle texture;
        DirectX::XMFLOAT2 translation;
        bool scissorEnable;
        vk::Rect2D scissor;
        DirectX::XMFLOAT4X4 transform;
    };

    struct RenderTextureEntry
    {
        std::shared_ptr<RxCore::Image> image;
        std::shared_ptr<RxCore::ImageView> imageView;
        vk::Sampler sampler;
    };

    struct RmlPushConstantData {
        DirectX::XMFLOAT2 translate;
        uint32_t textureId;
        uint32_t pad;
        DirectX::XMFLOAT4X4 transform;
    };

    class RmlRenderInterface final : public Rml::RenderInterface
    {
    public:
        RmlRenderInterface();
        ~RmlRenderInterface() override;
        void RenderGeometry(
            Rml::Vertex * vertices,
            int numVertices,
            int * indices,
            int numIndices,
            Rml::TextureHandle texture,
            const Rml::Vector2f & translation) override;

        void EnableScissorRegion(bool enable) override;
        void SetScissorRegion(int x, int y, int width, int height) override;

        bool LoadTexture(
            Rml::TextureHandle & texture_handle,
            Rml::Vector2i & texture_dimensions,
            const Rml::String & source) override;

        bool GenerateTexture(
            Rml::TextureHandle & texture_handle,
            const Rml::byte * source,
            const Rml::Vector2i & source_dimensions) override;
        void ReleaseTexture(Rml::TextureHandle texture) override;

        void SetTransform(const Rml::Matrix4f * transform) override;

        void resetRender();
        void renderUi(ecs::World* world);
        void setDirty() { dirtyTextures = true;}

    private:

        std::tuple<std::shared_ptr<RxCore::VertexBuffer>, std::shared_ptr<RxCore::IndexBuffer>> CreateBuffers() const;

        std::unordered_map<uintptr_t, RenderTextureEntry> textureEntries_;
        bool dirtyTextures{true};

        uintptr_t getNextTextureHandle() const;
        bool scissorEnabled_{};
        vk::Rect2D scissorRect;
        DirectX::XMFLOAT4X4 transform_{};
        std::vector<UiRenderEntry> renders;
        std::vector<Rml::Vertex> vertices_;
        std::vector<uint32_t> indices_;

        RxCore::DescriptorPoolTemplate poolTemplate_;
        std::shared_ptr<RxCore::DescriptorSet> currentDescriptorSet;
        std::unordered_map<uintptr_t, uint32_t> textureSamplerMap;

        std::shared_ptr<RxCore::DescriptorSet> set0_;
        ecs::EntityHandle pipeline_;

        std::shared_ptr<RxCore::Buffer> ub_;
        DirectX::XMFLOAT4X4 projectionMatrix_{};
    };
}

#endif //RX_RMLRENDERINTERFACE_H
