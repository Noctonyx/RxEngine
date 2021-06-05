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

#ifndef INDUSTRONAUT_SWAPCHAIN_H
#define INDUSTRONAUT_SWAPCHAIN_H

#include <Modules/Module.h>
#include <RxCore.h>

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
        RxApi::ImageViewPtr imageView;
        RxApi::Semaphore imageAvailableSempahore;
        uint32_t imageIndex;
        RxApi::Extent extent;
        RxApi::Semaphore finishRenderSemaphore;
    };

    struct MainRenderImageOutput
    {
        RxApi::ImageViewPtr imageView;
        RxApi::Semaphore finishRenderSemaphore;
    };

    class SwapChainModule : public Module
    {
    public:
        SwapChainModule(ecs::World * world, EngineMain * engine, ecs::entity_t moduleId);
        void startup() override;
        void shutdown() override;
        [[nodiscard]] RxApi::ImageFormat getImageFormat() const;

    protected:
        void replaceSwapChain();
        void createSemaphores(uint32_t semaphoreCount);
        void destroySemaphores();

    private:
        std::unique_ptr<RxApi::SwapChain> swapChain_;
        std::vector<RxApi::Semaphore> submitCompleteSemaphores_;
    };
}

#endif //INDUSTRONAUT_SWAPCHAIN_H
