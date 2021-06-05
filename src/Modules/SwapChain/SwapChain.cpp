////////////////////////////////////////////////////////////////////////////////
// MIT License
//
// Copyright (c) 2021.  Shane Hyde
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

//
// Created by shane on 5/06/2021.
//

#include "SwapChain.h"
#include "EngineMain.hpp"
#include "RxECS.h"
#include "optick/optick.h"

namespace RxEngine
{
    SwapChainModule::SwapChainModule(ecs::World * world, EngineMain * engine, const ecs::entity_t moduleId)
        : Module(world, engine, moduleId)
    {
        auto surface = engine->getDevice()->getSurface();
        swapChain_ = surface->CreateSwapChain();
        swapChain_->setSwapChainOutOfDate(true);
    }

    void SwapChainModule::startup()
    {
        world_->createSystem("SwapChain:CheckSwapchain")
              .inGroup("Pipeline:PreFrame")
              .execute(
                  [this](ecs::World *) {
                      OPTICK_EVENT("Check SwapChain")
                      if (swapChain_->swapChainOutOfDate()) {
                          replaceSwapChain();
                      }
                  }
              );

        world_->createSystem("SwapChain:AcquireImage")
              .inGroup("Pipeline:PostRender")
              .label<AcquireImage>()
              .withWrite<MainRenderImageInput>()
              .execute(
                  [this](ecs::World * w) {
                      OPTICK_EVENT("AcquireImage")
                      const auto current_extent = swapChain_->getExtent();

                      auto[next_swap_image_view, next_image_available, next_image_index] =
                      swapChain_->acquireNextImage();

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
                  [this](ecs::World *, const MainRenderImageOutput * mri) {
                      OPTICK_GPU_FLIP(nullptr)
                      OPTICK_CATEGORY("Present", Optick::Category::Rendering)

                      swapChain_->presentImage(mri->imageView, mri->finishRenderSemaphore);
                      return true;
                  }
              );
    }

    void SwapChainModule::shutdown()
    {
        destroySemaphores();

        swapChain_.reset();
    }

    void SwapChainModule::replaceSwapChain()
    {
        auto device = engine_->getDevice();
        device->waitIdle();

        if (swapChain_->imageCount() != submitCompleteSemaphores_.size()) {
            destroySemaphores();
            createSemaphores(swapChain_->imageCount());
        }

        swapChain_.reset();
        device->surface->updateSurfaceCapabilities();
        swapChain_ = device->surface->CreateSwapChain();
    }

    void SwapChainModule::createSemaphores(uint32_t semaphoreCount)
    {
        for (uint32_t i = 0; i < semaphoreCount; i++) {
            auto s = RxCore::Device::VkDevice().createSemaphore({});
            submitCompleteSemaphores_.push_back(s);
        }
    }

    void SwapChainModule::destroySemaphores()
    {
        for (const auto & semaphore: submitCompleteSemaphores_) {
            RxCore::Device::VkDevice().destroySemaphore(semaphore);
        }
        submitCompleteSemaphores_.clear();
    }

    vk::Format SwapChainModule::getImageFormat() const
    {
        return swapChain_->imageFormat();
    }
}
