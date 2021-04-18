#pragma once

#include <vector>
#include <memory>
#include <optional>
#include "RenderData.h"
#include <Jobs/JobManager.hpp>
#include "ILightingManager.h"
#include "MaterialManager.h"
#include "EntityManager.h"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DeviceObject.h"
#include "MeshBundle.h"
#include "RenderCamera.h"
#include "DirectXCollision.h"
#include "RxECS.h"
#include "Modules/Render.h"

#define NUM_CASCADES 4

namespace RxCore
{
    class FrameBuffer;
    class RenderPass;
    class Device;
    class Buffer;
    class Image;
    class ImageView;
    class DescriptorPool;
    class Queue;
    class CommandPool;
    class PrimaryCommandBuffer;
    class CommandBuffer;
}

namespace RxEngine
{
    class World;
    class BatchManager;
    struct ShadowCascade;
    class Renderer;
    class Camera;
    class SceneCamera;
    class Lighting;

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
        EntityId entityId;
        DirectX::XMFLOAT4X4 transform;
        DirectX::BoundingSphere boundsSphere;
        DirectX::XMFLOAT4 instanceParams1;
#if 0
        DirectX::XMFLOAT4 instanceParams2;
        DirectX::XMFLOAT4 instanceParams3;
        DirectX::XMFLOAT4 instanceParams4;
#endif
    };

    struct RenderEntityEntry
    {
        std::shared_ptr<MeshBundle> bundle;
        uint32_t lodCount;
        std::array<std::pair<uint32_t, float>, 4> lods;
        std::vector<RenderEntityInstance> instances;

        uint32_t materialId;
        std::array<uint32_t, 3> pipelineIds;
    };

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
        DirectX::XMFLOAT4 params;
        uint32_t materialId;
        uint32_t pad0;
        uint32_t pad1;
        uint32_t pad2;
    };

    struct IndirectDrawCommandHeader
    {
        MaterialPipelineId pipelineId;
        MeshBundle * bundle;
        uint32_t commandStart;
        uint32_t commandCount;
    };

    struct IndirectDrawSet
    {
        std::vector<IndirectDrawCommand> commands;
        std::vector<IndirectDrawInstance> instances;

        std::vector<IndirectDrawCommandHeader> headers;
    };

    struct IRenderProvider
    {
    public:
        virtual void setup(const std::shared_ptr<Camera> & camera) = 0;
        virtual void teardown() = 0;

        virtual void updateDescriptor(
            const std::shared_ptr<RxCore::DescriptorSet> & set,
            uint32_t binding) const {}
    };
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

    class Renderer : public RxCore::DeviceObject
    {
    public:
        explicit Renderer(vk::Device device, ecs::World * world);

        ~Renderer();

        void startup(vk::Format imageFormat);

        void render(
            const std::shared_ptr<RenderCamera> & camera,
            const std::vector<IRenderable *> & subsystems,
            vk::ImageView imageView,
            vk::Extent2D extent,
            std::vector<vk::Semaphore> waitSemaphores,
            std::vector<vk::PipelineStageFlags> waitStages,
            vk::Semaphore completeSemaphore
        );
        void shutdown();

        [[nodiscard]] RenderStage getSequenceRenderStage(ERenderSequence seq) const;
        void setLightingManager(std::shared_ptr<Lighting> lm);
        //void setTextureBundle(const std::shared_ptr<TextureBundle> & textureBundle);

    protected:
        void createRenderPass();
        void ensureMaterialPipelinesExist();
        void createDepthRenderPass();

        std::shared_ptr<const std::vector<RenderEntity>> finishUpEntityJobs(
            const std::vector<std::shared_ptr<RxCore::Job<std::vector<RenderEntity>>>> & entityJobs);

        void createShadowJobs(
            std::vector<std::shared_ptr<RxCore::Job<RenderResponse>>> & jobs,
            const std::vector<IRenderable *> & subsystems,
            const std::vector<ShadowCascade> & cascades,
            uint32_t
            activeCascade,
            const uint32_t width,
            const uint32_t height
        );

        void createOpaqueJobs(
            std::vector<std::shared_ptr<RxCore::Job<RenderResponse>>> & jobs,
            const std::vector<IRenderable *> & subsystems,
            const std::shared_ptr<Camera> & camera,
            const uint32_t width,
            const uint32_t height
        );

        void createUiJobs(
            std::vector<std::shared_ptr<RxCore::Job<RenderResponse>>> & jobs,
            const std::vector<IRenderable *> & subsystems,
            const uint32_t width,
            const uint32_t height
        );

        void waitAndFinishJobs(
            std::vector<std::shared_ptr<RxCore::Job<RenderResponse>>> & jobs,
            uint16_t seq,
            std::shared_ptr<RxCore::PrimaryCommandBuffer>
            & buf
        );

        void renderIndirectDraws(
            IndirectDrawSet ids,
            const std::shared_ptr<RxCore::SecondaryCommandBuffer> & buf) const;

        void setScissorAndViewport(
            vk::Extent2D extent,
            std::shared_ptr<RxCore::SecondaryCommandBuffer> buf,
            bool flipY) const;

        RenderResponse renderOpaque(
            vk::Extent2D extent,
            const std::shared_ptr<Camera> & camera,
            std::shared_ptr<const std::vector<RenderEntity>> entities);

        RenderResponse renderShadow(
            vk::Extent2D extent,
            const DirectX::XMMATRIX & viewProj,
            const DirectX::XMMATRIX & view,
            const DirectX::BoundingOrientedBox& cullBox,
            uint32_t cascadeIndex,
            std::shared_ptr<const std::vector<RenderEntity>> entities);

        RenderResponse renderSelectedEntities(
            vk::Extent2D extent,
            const DirectX::XMMATRIX & projView,
            const DirectX::XMMATRIX & view,
            const std::shared_ptr<const std::vector<RenderEntity>> & entities,
            std::vector<uint32_t> selectedEntitiesIndex,
            uint32_t pipelineIndex,
            bool flipY,
            const vk::RenderPass & pass,
            int subPass,
            uint32_t pushOffset,
            uint32_t pushSize,
            void * pushData
        );
        

    public:
        void updateGui() const;
        //void collectLights(const std::vector<IRenderable *> & subsystems, std::vector<LightData> & lights);
    public:
    public:
        double gpuTime{};
        double cpuTime{};

    private:
        vk::Format imageFormat_;

        vk::Extent2D bufferExtent_;
        vk::QueryPool queryPool_;

        std::shared_ptr<RxCore::Image> depthBuffer_;
        std::shared_ptr<RxCore::Image> shadowMap_;
        std::shared_ptr<RxCore::ImageView> wholeShadowMapView_;
        std::vector<std::shared_ptr<RxCore::ImageView>> cascadeViews_;
        std::vector<std::shared_ptr<RxCore::FrameBuffer>> cascadeFrameBuffers_;
        std::shared_ptr<RxCore::ImageView> depthBufferView_;
        std::shared_ptr<RxCore::CommandPool> graphicsCommandPool_;

        vk::RenderPass renderPass_;
        vk::RenderPass depthRenderPass_;

        vk::DescriptorSetLayout ds0Layout;
        std::shared_ptr<RxCore::DescriptorSet> ds0_;
        bool shadowImagesChanged;

        vk::Sampler shadowSampler_;

        std::shared_ptr<Lighting> lightingManager_;
        vk::PipelineLayout pipelineLayout;
        std::vector<vk::DescriptorSetLayout> dsLayouts;

        void ensureDepthBufferExists(vk::Extent2D & extent);
        void ensureShadowImages(uint32_t shadowMapSize, uint32_t numCascades);

        RxCore::DescriptorPoolTemplate poolTemplate;
        //void ensureFrameBufferSize(const vk::ImageView & imageView, const vk::Extent2D & extent);

        std::shared_ptr<RxCore::FrameBuffer> createRenderFrameBuffer(
            const vk::ImageView & imageView,
            const vk::Extent2D & extent) const;

        void createPipelineLayout();
        void updateDescriptorSet0(const std::shared_ptr<RenderCamera> & renderCamera);
#if 0
        void cullAndLodEntities(
            const glm::mat4 & projView,
            const std::shared_ptr<const std::vector<RenderEntityEntry>> & entities,
            std::vector<uint32_t> selectedEntities,
            uint32_t pipelineIndex,
            IndirectDrawSet & drawSet) const;
#endif
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

        std::vector<std::shared_ptr<RxCore::Buffer>> instanceBuffers;
        std::vector<std::shared_ptr<RxCore::DescriptorSet>> instanceBUfferDS;
        uint32_t ibCycle{};
        std::mutex ibLock{};
        ecs::World* world_;
        //flecs::world* world_;
        //flecs::query<const Render::MaterialPipelineDetails> query_;
    };
} // namespace RXCore
