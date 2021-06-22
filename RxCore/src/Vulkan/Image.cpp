////////////////////////////////////////////////////////////////////////////////
// MIT License
//
// Copyright (c) 2021-2021.  Shane Hyde (shane@noctonyx.com)
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

#include "Image.hpp"
//#include "vulkan/vulkan.hpp"
#include "Device.h"

namespace RxCore
{
    Image::~Image()
    {
        device_->destroyImage(this);
        allocation_.reset();
    }
#if 0
    std::shared_ptr<ImageView> Image::createImageView(
        VkImageViewType viewType,
        VkImageAspectFlagBits aspect,
        uint32_t baseArrayLayer,
        uint32_t layerCount)
    {
        VkImageViewCreateInfo ivci;

        ivci.setViewType(viewType)
            .setFormat(format_)
            .setImage(handle_)
            .setSubresourceRange(
                {aspect, 0, VK_REMAINING_MIP_LEVELS, baseArrayLayer, layerCount}
            );

        auto h = device_->getDevice().createImageView(ivci);
        return std::make_shared<ImageView>(device_, h, shared_from_this());
    }
#endif
    ImageView::ImageView(const Device * device, VkImageView handle)
        : handle_(handle)
          , device_(device)
          //, image_(std::move(image))
    {}

    ImageView::~ImageView()
    {
        device_->destroyImageView(this);
       // image_.reset();
    }
#if 0
    std::shared_ptr<Image> ImageView::getImage() const
    {
        return image_;
    }
#endif
} // namespace RXCore
