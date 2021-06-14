//
// Created by shane on 5/06/2021.
//

#ifndef RXENGINE_SWAPCHAIN_H
#define RXENGINE_SWAPCHAIN_H

#include <Modules/Module.h>
#include <Vulkan/SwapChain.hpp>
#include "Vulkan/DescriptorSet.hpp"

namespace RxEngine
{
    struct AcquireImage
    {
    };
    struct PresentImage
    {
    };

    struct MainRenderImageInput
    {
        vk::ImageView imageView;
        vk::Semaphore imageAvailableSempahore;
        uint32_t imageIndex;
        vk::Extent2D extent;
        vk::Semaphore finishRenderSemaphore;
    };

    struct MainRenderImageOutput
    {
        vk::ImageView imageView;
        vk::Semaphore finishRenderSemaphore;
    };

    class SwapChainModule : public Module
    {
    public:
        SwapChainModule(ecs::World * world, EngineMain * engine, ecs::entity_t moduleId);
        void startup() override;
        void shutdown() override;
        [[nodiscard]] vk::Format getImageFormat() const;

    protected:
        void replaceSwapChain();
        void createSemaphores(uint32_t semaphoreCount);
        void destroySemaphores();

    private:
        std::unique_ptr<RxCore::SwapChain> swapChain_;
        std::vector<vk::Semaphore> submitCompleteSemaphores_;
    };
}

#endif //RXENGINE_SWAPCHAIN_H
