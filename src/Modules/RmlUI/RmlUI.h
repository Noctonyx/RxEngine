#pragma once
#include "Modules/Module.h"
#include <RxCore.h>
#include "DirectXMath.h"
#include "Vfs.h"
#include "RmlUi/Core/FileInterface.h"
#include "RmlUi/Core/Log.h"
#include "RmlUi/Core/RenderInterface.h"
#include "RmlUi/Core/SystemInterface.h"

namespace Rml
{
    struct Vertex;
    class ElementDocument;
}

namespace RxEngine
{
#if 0
    struct UiContext
    {
        Rml::Context* context;
        bool debugger = false;
        //bool interactive = false;
        std::unordered_map<std::string, Rml::ElementDocument*> documents{};

        Rml::ElementDocument* loadDocument(const std::string& document);
        void closeDocument(const std::string& document);
    };

    struct DestroyUi {};
    struct UiContextCreated {};
    struct UiContextInteractive {};
    struct UiContextProcessed {};

#endif
    class RmlSystemInterface : public Rml::SystemInterface
    {
    public:
        RmlSystemInterface(ecs::World * world);

        bool LogMessage(Rml::Log::Type type, const Rml::String & message) override;
        double GetElapsedTime() override;
        int TranslateString(Rml::String & translated, const Rml::String & input) override;

    private:
        ecs::World * world_;
    };

    struct UiRenderEntry
    {
        size_t vertexOffset;
        size_t indexOffset;
        size_t indexCount;

        Rml::TextureHandle texture;
        DirectX::XMFLOAT2 translation;
        bool scissorEnable;
        RxApi::Rect scissor;
        DirectX::XMFLOAT4X4 transform;
    };

    struct RenderTextureEntry
    {
       RxApi::ImagePtr image;
        RxApi::ImageViewPtr imageView;
        RxApi::Sampler sampler;
    };

    struct RmlPushConstantData
    {
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
        void renderUi(ecs::World * world);

        void setDirty()
        {
            dirtyTextures = true;
        }

    private:
        std::tuple<RxApi::VertexBufferPtr ,RxApi::IndexBufferPtr>
        CreateBuffers() const;

        std::unordered_map<uintptr_t, RenderTextureEntry> textureEntries_;
        bool dirtyTextures{true};

        uintptr_t getNextTextureHandle() const;
        bool scissorEnabled_{};
        RxApi::Rect scissorRect;
        DirectX::XMFLOAT4X4 transform_{};
        std::vector<UiRenderEntry> renders;
        std::vector<Rml::Vertex> vertices_;
        std::vector<uint32_t> indices_;

        RxApi::DescriptorSetPtr currentDescriptorSet;
        std::unordered_map<uintptr_t, uint32_t> textureSamplerMap;

        RxApi::DescriptorSetPtr set0_;
        ecs::EntityHandle pipeline_{};
        std::shared_ptr<RxCore::DescriptorPool> descriptorPool;

        RxApi::UniformBufferPtr ub_;
        DirectX::XMFLOAT4X4 projectionMatrix_{};
    };

    struct FileHandle
    {
        std::vector<std::byte> buffer;
        size_t size;
        size_t pointer;
    };

    class RmlFileInterface : public Rml::FileInterface
    {
    public:
        RmlFileInterface();

        Rml::FileHandle Open(const Rml::String & path) override;
        void Close(Rml::FileHandle file) override;
        size_t Read(void * buffer, size_t size, Rml::FileHandle file) override;
        bool Seek(Rml::FileHandle file, long offset, int origin) override;
        size_t Tell(Rml::FileHandle file) override;

    private:
        //EngineMain * engine_;
        RxAssets::Vfs * vfs_;
        uintptr_t nextHandle = 1;
        std::unordered_map<uintptr_t, FileHandle> fileData;
    };

    class RmlUiModule : public Module
    {
    public:
        RmlUiModule(ecs::World * world, EngineMain * engine, const ecs::entity_t moduleId)
            : Module(world, engine, moduleId) {}

        void startup() override;
        void shutdown() override;

    protected:
        std::unique_ptr<RmlSystemInterface> rmlSystem;
        std::unique_ptr<RmlFileInterface> rmlFile;
        std::unique_ptr<RmlRenderInterface> rmlRender;

        Rml::Context * mainUI;
    };
}
