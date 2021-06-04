#pragma once

#include <vector>
#include <memory>
//#include "Vulkan/DescriptorSet.hpp"
//#include "Vulkan/DeviceObject.h"
#include "DirectXCollision.h"
#include "RxECS.h"
#include "Modules/Module.h"
#include "Modules/Materials/Materials.h"
//#include <Jobs/JobManager.hpp>

//#include "Vulkan/DescriptorPool.hpp"

#define NUM_CASCADES 4

namespace RxEngine
{
    struct ShadowCascade;
    class Renderer;
    class Camera;
    //class Lighting;

    enum ELightType
    {
        Directional,
        Spot,
        Point
    };

    class IRenderable;

    struct LightData
    {
        ELightType lightType;

        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 direction;

        float intensity;
        DirectX::XMFLOAT3 color;

        IRenderable * origin;
    };

    struct RenderEntityInstance
    {
        DirectX::XMFLOAT4X4 transform;
        DirectX::BoundingSphere boundsSphere;
        DirectX::XMFLOAT4 instanceParams1;
#if 0
        DirectX::XMFLOAT4 instanceParams2;
        DirectX::XMFLOAT4 instanceParams3;
        DirectX::XMFLOAT4 instanceParams4;
#endif
    };

    struct RenderEntity
    {
        //EntityId entityId;
        DirectX::XMFLOAT4X4 transform;
        DirectX::BoundingSphere boundsSphere;
        DirectX::XMFLOAT4 instanceParams1;
#if 0
        DirectX::XMFLOAT4 instanceParams2;
        DirectX::XMFLOAT4 instanceParams3;
        DirectX::XMFLOAT4 instanceParams4;
#endif
    };
#if 0
    struct RenderEntityEntry
    {
        std::shared_ptr<MeshBundle> bundle;
        uint32_t lodCount;
        std::array<std::pair<uint32_t, float>, 4> lods;
        std::vector<RenderEntityInstance> instances;

        uint32_t materialId;
        std::array<uint32_t, 3> pipelineIds;
    };
#endif
    struct IndirectDrawCommand
    {
        uint32_t indexCount;
        uint32_t vertexOffset;
        uint32_t indexOffset;
        uint32_t instanceCount;
        uint32_t instanceOffset;
    };

    struct IndirectDrawInstance
    {
        DirectX::XMFLOAT4X4 transform;
        //DirectX::XMFLOAT4 params;
        uint32_t materialId;
        uint32_t pad0;
        uint32_t pad1;
        uint32_t pad2;
    };

    struct IndirectDrawCommandHeader
    {
        ecs::entity_t pipelineId;
        ecs::entity_t bundle;
        uint32_t commandStart;
        uint32_t commandCount;
    };

    struct IndirectDrawSet
    {
        std::vector<IndirectDrawCommand> commands;
        std::vector<IndirectDrawInstance> instances;

        std::vector<IndirectDrawCommandHeader> headers;
    };

    struct AcquireImage {};

    struct PresentImage {};

    struct MainRenderImageInput
    {
        RxApi::ImageView imageView;
        RxApi::Semaphore imageAvailableSempahore;
        uint32_t imageIndex;
        RxApi::Extent extent;
        RxApi::Extent finishRenderSemaphore;
    };

    struct MainRenderImageOutput
    {
        RxApi::ImageView imageView;
        RxApi::Semaphore finishRenderSemaphore;
    };

    struct UiCommandBuffer
    {
        RxApi::SecondaryCommandBufferPtr buf;
    };

    struct FrameStatDetail
    {
        float cpuTime;
    };

    struct FrameStats
    {
        std::vector<FrameStatDetail> frames;
        uint64_t frameNo;
        uint32_t index;
    };
#if 0
    struct IRenderProvider
    {
    public:
        virtual void setup(const std::shared_ptr<Camera> & camera) = 0;
        virtual void teardown() = 0;

        virtual void updateDescriptor(
            const std::shared_ptr<RxCore::DescriptorSet> & set,
            uint32_t binding) const {}
    };
#endif
#if 0
    class IRenderable
    {
    public:
        virtual void rendererInit(Renderer * renderer) = 0;

        virtual ~IRenderable() = default;

        //virtual void lighting(const std::shared_ptr<Camera> & camera) {}
        virtual void preRender(uint32_t width, uint32_t height) {}

#if 0
        [[nodiscard]] virtual bool hasRenderShadows() const
        {
            return false;
        }

        [[nodiscard]] virtual bool hasRenderOpaque() const
        {
            return false;
        }
#endif

        [[nodiscard]] virtual bool hasRenderUi() const
        {
            return false;
        }
#if 0

        virtual void getLights(std::vector<LightData> & lights) {}
        [[nodiscard]] virtual RenderResponse renderShadows(
            const std::vector<ShadowCascade> & cascades,
            const RenderStage & stage,
            const uint32_t activeCascade,
            const uint32_t width,
            const uint32_t height
        )
        {
            return {};
        }

        [[nodiscard]] virtual RenderResponse renderOpaque(
            const std::shared_ptr<Camera> & camera,
            const RenderStage & stage,
            const uint32_t width,
            const uint32_t height
        )
        {
            return {};
        }
#endif
        [[nodiscard]] virtual RenderResponse renderUi(
            const RenderStage & stage,
            const uint32_t width,
            const uint32_t height
        )
        {
            return {};
        }

        virtual void postRender() {}

        static void setScissorAndViewPort(
            const std::shared_ptr<RxCore::CommandBuffer> & buf,
            uint32_t width,
            uint32_t height,
            bool flipY);

        virtual std::vector<RenderEntity> getRenderEntities() = 0;
    };
#endif
    struct RenderPushConstants
    {
        DirectX::XMFLOAT4X4 localMatrix;
        uint32_t cascadeIndex;
    };

    struct RenderPasses
    {
        RxApi::RenderPass opaqueRenderPass{};
        uint32_t opaqueSubPass;
        RxApi::RenderPass shadowRenderPass{};
        uint32_t shadowSubPass;
        RxApi::RenderPass transparentRenderPass{};
        uint32_t transparentSubPass;
        RxApi::RenderPass uiRenderPass{};
        uint32_t uiSubPass;
    };

    struct DescriptorSet
    {
        RxApi::DescriptorSetPtr ds{};
    };

    struct CurrentMainDescriptorSet
    {
        ecs::entity_t descriptorSet;
    };
#if 0
    struct Descriptors
    {
        bool newDescriptor;
        std::shared_ptr<RxCore::DescriptorSet> ds0 {};
    };
#endif
    class Renderer : public Module
    {
    public:
        explicit Renderer(RxApi::DevicePtr device,
                          ecs::World * world,
                          RxApi::ImageFormat imageFormat,
                          EngineMain * engine, const ecs::entity_t moduleId);

        ~Renderer();


        void startup();

        void render(
            //const std::shared_ptr<RenderCamera> & camera,
            //const std::vector<IRenderable *> & subsystems,
            RxApi::ImageViewPtr imageView,
            RxApi::Extent extent,
            std::vector<RxApi::Semaphore> waitSemaphores,
            std::vector<RxApi::PipelineStageFlags> waitStages,
            RxApi::Semaphore completeSemaphore
        );
        void shutdown();

        //[[nodiscard]] RenderStage getSequenceRenderStage(ERenderSequence seq) const;
        //void setLightingManager(std::shared_ptr<Lighting> lm);
        //void setTextureBundle(const std::shared_ptr<TextureBundle> & textureBundle);

    protected:
        void createRenderPass();
        void createDepthRenderPass();
#if 0
        std::shared_ptr<const std::vector<RenderEntity>> finishUpEntityJobs(
            const std::vector<std::shared_ptr<RxCore::Job<std::vector<RenderEntity>>>> &
            entityJobs);
#endif
        void setScissorAndViewport(
            RxApi::Extent extent,
            RxApi::SecondaryCommandBufferPtr buf,
            bool flipY) const;
#if 0
        vk::Pipeline createUiMaterialPipeline(const MaterialPipelineDetails * mpd,
                                              const FragmentShader * frag,
                                              const VertexShader * vert,
                                              vk::PipelineLayout layout,
                                              vk::RenderPass rp,
                                              uint32_t subpass);
#endif
    public:
        //void collectLights(const std::vector<IRenderable *> & subsystems, std::vector<LightData> & lights);
    public:
    public:
        double gpuTime{};
        double cpuTime{};

    private:
        RxApi::ImageFormat imageFormat_;

        RxApi::Extent bufferExtent_;
        //vk::QueryPool queryPool_;

        RxApi::ImagePtr depthBuffer_;
        RxApi::ImagePtr  shadowMap_;
        RxApi::ImageViewPtr  wholeShadowMapView_;
        std::vector<RxApi::ImageView> cascadeViews_;
        std::vector<RxApi::FrameBufferPtr> cascadeFrameBuffers_;
        RxApi::ImageViewPtr depthBufferView_;
        RxApi::CommandPoolPtr graphicsCommandPool_;

        RxApi::RenderPass renderPass_;
        RxApi::RenderPass depthRenderPass_;

        RxApi::DescriptorSetLayout ds0Layout;
        RxApi::DescriptorSetPtr ds0_;
        bool shadowImagesChanged;

        RxApi::Sampler shadowSampler_;

        //std::shared_ptr<Lighting> lightingManager_;
        //vk::PipelineLayout pipelineLayout;
        //std::vector<vk::DescriptorSetLayout> dsLayouts;

        RxApi::DescriptorPoolPtr descriptorPool;
        void ensureDepthBufferExists(RxApi::Extent & extent);
        void ensureShadowImages(uint32_t shadowMapSize, uint32_t numCascades);

         ///     RxCore::DescriptorPoolTemplate poolTemplate;
        //void ensureFrameBufferSize(const vk::ImageView & imageView, const vk::Extent2D & extent);

        RxApi::FrameBufferPtr createRenderFrameBuffer(
            const RxApi::ImageView & imageView,
            const RxApi::Extent & extent) const;

        //void createPipelineLayout();
        //void updateDescriptorSet0(const std::shared_ptr<RenderCamera> & renderCamera);
#if 0
        void cullAndLodEntities(
            const glm::mat4 & projView,
            const std::shared_ptr<const std::vector<RenderEntityEntry>> & entities,
            std::vector<uint32_t> selectedEntities,
            uint32_t pipelineIndex,
            IndirectDrawSet & drawSet) const;
#endif
#if 0
        static void cullEntitiesProj(
            const DirectX::XMMATRIX & proj,
            const DirectX::XMMATRIX & view,
            const std::shared_ptr<const std::vector<RenderEntity>> & entities,
            std::vector<uint32_t> & selectedEntities);

        static void cullEntitiesOrtho(
            const DirectX::BoundingOrientedBox & cullBox,
            const std::shared_ptr<const std::vector<RenderEntity>> & entities,
            std::vector<uint32_t> & selectedEntities);

        static float getSphereSize(
            const DirectX::XMMATRIX & projView,
            const DirectX::XMVECTOR & viewRight,
            const DirectX::BoundingSphere & sphere);
#endif
        std::vector<RxApi::BufferPtr> instanceBuffers;
        std::vector<RxApi::DescriptorSetPtr> instanceBufferDS;
        //uint32_t ibCycle{};
        std::mutex ibLock{};
        //ecs::World* world_;
        //flecs::world* world_;
        //flecs::query<const Render::MaterialPipelineDetails> query_;
    };
} // namespace RXCore
