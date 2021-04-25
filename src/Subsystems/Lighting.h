#pragma once

//#include "Subsystem.h"
//#include "glm/vec3.hpp"

#include "Vulkan/DescriptorSet.hpp"
#include "Geometry/Camera.hpp"
#include "Pooler.h"
//#include "Rendering/Renderer.hpp"

namespace RxCore
{
    struct LightData;
}

namespace RxEngine
{

#define MAX_SHADOW_CASCADES 4

#if 0

    class Lighting
        : public IRenderProvider,
          public IRenderable,
          public ILightingManager,
          public std::enable_shared_from_this<Lighting>
    {
    public:

        explicit Lighting()
            : ix(0)
        {
            mainLightDirection = {0.407f, -.707f, 0.8f};
           // mainLightDirection = {0.f, -1.f, 0.0f};
#if 0
            pool.setAllocator(
                [=]
                {
                    auto b = RXCore::iVulkan()->createBuffer(
                        vk::BufferUsageFlagBits::eUniformBuffer,
                        VMA_MEMORY_USAGE_CPU_TO_GPU,
                        sizeof(lighting_));
                    b->getMemory()->map();
                    return b;
                });
#endif
            bufferAlignment = RxCore::iVulkan()->getUniformBufferAlignment(sizeof(lighting_));

            lightingBuffer2_ = RxCore::iVulkan()->createBuffer(
                vk::BufferUsageFlagBits::eUniformBuffer,
                VMA_MEMORY_USAGE_CPU_TO_GPU, 10 * bufferAlignment);
            lightingBuffer2_->getMemory()->map();
        };

//        int16_t GetCallPriority(Sequences sequence) override;
        void UpdateGui();

        void setup(const std::shared_ptr<Camera> & camera) override;
        void teardown() override;

        void rendererInit(Renderer * renderer) override;

        //void lighting(const std::shared_ptr<RXCore::Camera> & camera) override;
        //void preRender(uint32_t width, uint32_t height) override;
        //void postRender() override;
        void updateDescriptor(
            const std::shared_ptr<RxCore::DescriptorSet> & set,
            uint32_t binding) const override;

        void updateShadowMapDescriptor(
            const std::shared_ptr<RxCore::DescriptorSet> & set,
            uint32_t binding
        );
        uint32_t getDescriptorOffset() const { return static_cast<uint32_t>((ix * bufferAlignment)); }
        //void getBuffer();
        //void getLights(std::vector<RXCore::LightData> & lights) override;

        //void setShadowData(const uint32_t numCascades, std::vector<RXCore::ShadowCascade> cascades);
        void prepareCamera(const std::shared_ptr<Camera> & camera) override;
        void getCascadeData(std::vector<ShadowCascade> & cascades) override;
        void setShadowMap(std::shared_ptr<RxCore::ImageView> shadowMap) override;
 //       void removedFromScene() override;

        bool hasShadowMap() const { return static_cast<bool>(shadowMap_); }

        void createShadowCascadeViews(
            const std::shared_ptr<Camera> & camera,
            const uint32_t numCascades,
            std::vector<ShadowCascade> & cascades,
            const DirectX::XMFLOAT3 lightDirection) const;

        std::vector<RenderEntity> getRenderEntities() override;
    private:
        struct
        {
            float ambientStrength;
            float diffAmount;
            float specularStrength;
            float specularPower;
            DirectX::XMFLOAT3 light_direction;
            uint32_t cascadeCount;
            //glm::uint cascadeDraw;
            struct ShadowCascadeShader cascades[MAX_SHADOW_CASCADES];
        } lighting_{};

        std::vector<ShadowCascade> cascades_;
        DirectX::XMFLOAT3 mainLightDirection;
        //glm::vec3 sunLightDirection{};

        //DirectX::XMFLOAT3 dir{};
        //std::shared_ptr<RXCore::Buffer> lightingBuffer_;
        std::shared_ptr<RxCore::Buffer> lightingBuffer2_;
        std::shared_ptr<RxCore::ImageView> shadowMap_;
        vk::Sampler shadowSampler_;

        size_t bufferAlignment;
        // std::deque<std::shared_ptr<RXCore::Buffer>> lightingBuffers_;
        //RXUtil::Pooler<RXCore::Buffer> pool;

        //Renderer * renderer_{};
        uint32_t ix;
    public:
    };
#endif
}
