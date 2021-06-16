//
// Created by shane on 5/06/2021.
//

#include "SwapChain.h"
#include "EngineMain.hpp"
#include "RxECS.h"
#include "optick/optick.h"

namespace RxEngine
{
    SwapChainModule::SwapChainModule(ecs::World * world,
                                     EngineMain * engine,
                                     const ecs::entity_t moduleId)
        : Module(world, engine, moduleId)
    {
        //auto device = engine->getDevice();

        //auto surface = engine->getDevice()->surface;
        //swapChain_ = device->createSwapChain();
        //swapChain_->setSwapChainOutOfDate(true);
    }

    void SwapChainModule::startup()
    {
        world_->createSystem("SwapChain:CheckSwapchain")
              .inGroup("Pipeline:PreFrame")
              .execute(
                  [this](ecs::World *)
                  {
                      OPTICK_EVENT("Check SwapChain")
                      if(engine_->getDevice()->checkSwapChain()) {
                          replaceSwapChain();
                      }
                  }
              );

        world_->createSystem("SwapChain:AcquireImage")
              .inGroup("Pipeline:PostRender")
              .label<AcquireImage>()
              .withWrite<MainRenderImageInput>()
              .execute(
                  [this](ecs::World * w)
                  {
                      OPTICK_EVENT("AcquireImage")
                      const auto current_extent = engine_->getDevice()->getSwapChainExtent();

                      auto [next_swap_image_view, next_image_available, next_image_index] =
                          engine_->getDevice()->acquireImage();

                      w->getStream<MainRenderImageInput>()->add<MainRenderImageInput>(
                          {
                              next_swap_image_view, next_image_available, next_image_index,
                              current_extent, submitCompleteSemaphores_[next_image_index]
                          }
                      );
                  }
              );

        world_->createSystem("SwapChain:PresentImage")
              .inGroup("Pipeline:PostRender")
              .label<PresentImage>()
              .withStream<MainRenderImageOutput>()
              .execute<MainRenderImageOutput>(
                  [this](ecs::World *, const MainRenderImageOutput * mri)
                  {
                      OPTICK_GPU_FLIP(nullptr)
                          OPTICK_CATEGORY("Present", Optick::Category::Rendering)

                          auto device = engine_->getDevice();
                      device->presentImage(mri->imageView, mri->finishRenderSemaphore);
                      //swapChain_->PresentImage(mri->imageView, mri->finishRenderSemaphore);
                      return true;
                  }
              );
        createSemaphores(engine_->getDevice()->getSwapChainImageCount());
    }

    void SwapChainModule::shutdown()
    {
        destroySemaphores();

        //swapChain_.reset();
    }

    void SwapChainModule::replaceSwapChain()
    {
        auto device = engine_->getDevice();
        device->WaitIdle();

        if (device->getSwapChainImageCount() != submitCompleteSemaphores_.size()) {
            destroySemaphores();
            createSemaphores(device->getSwapChainImageCount());
        }

        //swapChain_.reset();
        //device->updateSurfaceCapabilities();
        //swapChain_ = device->createSwapChain();
    }

    void SwapChainModule::createSemaphores(uint32_t semaphoreCount)
    {
        auto device = engine_->getDevice();

        for (uint32_t i = 0; i < semaphoreCount; i++) {
            auto s = device->createSemaphore();
            submitCompleteSemaphores_.push_back(s);
        }
    }

    void SwapChainModule::destroySemaphores()
    {
        auto device = engine_->getDevice();

        for (const auto & semaphore: submitCompleteSemaphores_) {
            device->destroySemaphore(semaphore);
        }
        submitCompleteSemaphores_.clear();
    }
#if 0
    VkFormat SwapChainModule::getImageFormat() const
    {
        return swapChain_->imageFormat();
    }
#endif
}
