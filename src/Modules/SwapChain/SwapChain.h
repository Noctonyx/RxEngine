//
// Created by shane on 5/06/2021.
//

#ifndef RXENGINE_SWAPCHAIN_H
#define RXENGINE_SWAPCHAIN_H

#include <Modules/Module.h>
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
        VkImageView imageView;
        VkSemaphore imageAvailableSempahore;
        uint32_t imageIndex;
        VkExtent2D extent;
        VkSemaphore finishRenderSemaphore;
    };

    struct MainRenderImageOutput
    {
        VkImageView imageView;
        VkSemaphore finishRenderSemaphore;
    };

    class SwapChainModule : public Module
    {
    public:
        SwapChainModule(ecs::World * world, EngineMain * engine, ecs::entity_t moduleId);
        void startup() override;
        void shutdown() override;
        //[[nodiscard]] VkFormat getImageFormat() const;

    protected:
        void replaceSwapChain();
        void createSemaphores(uint32_t semaphoreCount);
        void destroySemaphores();

    private:
        //std::unique_ptr<RxCore::SwapChain> swapChain_;
        std::vector<VkSemaphore> submitCompleteSemaphores_;
    };
}

#endif //RXENGINE_SWAPCHAIN_H
