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

#pragma once

#include "Allocation.h"
#include "Util.h"
#include "Device.h"

namespace RxCore
{
    class Device;

    class Buffer
    {
        friend class DescriptorSet;
        friend class Device;

    public:
        Buffer(Device * device,
               VkBuffer handle,
               std::shared_ptr<Allocation> allocation,
               VkDeviceSize size);

        virtual ~Buffer();

        RX_NO_COPY_NO_MOVE(Buffer)

        [[nodiscard]] VkBuffer handle() const
        {
            return handle_;
        }

        //[[nodiscard]] const std::shared_ptr<Allocation> & getMemory() const;
        [[nodiscard]] VkDeviceSize getSize() const;

        void map()
        {
            allocation_->map();
        }

        void unmap()
        {
            allocation_->unmap();
        }

        void update(const void* data, VkDeviceSize size) const
        {
            allocation_->update(data, size);
        }

        void update(const void* data, VkDeviceSize offset, VkDeviceSize size) const
        {
            allocation_->update(data, offset, size);
        }

        void * getPtr()
        {
            return allocation_->getPtr();
        }

        uint64_t getDeviceAddress() const
        {
            return device_->getBufferAddress(this);
        }
        
    private:
        Device * device_;
        const VkBuffer handle_ = nullptr;
        VkDeviceSize size_ = 0;
        std::shared_ptr<Allocation> allocation_;
    };
}
