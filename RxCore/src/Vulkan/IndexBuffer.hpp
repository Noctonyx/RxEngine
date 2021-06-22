#pragma once

#include "Buffer.hpp"

namespace RxCore
{
    class IndexBuffer : public Buffer, std::enable_shared_from_this<IndexBuffer>//, public IIndexBuffer
    {
        friend class CommandBuffer;

    public:
        explicit IndexBuffer(
            Device * device,
            VkBuffer handle,
            std::shared_ptr<Allocation> alloc,
            uint32_t indexCount,
            bool is16Bit
        )
            : Buffer(device, handle, alloc, (is16Bit ? sizeof(uint16_t) : sizeof(uint32_t)) * indexCount)
            , indexCount(indexCount)
            , is16Bit_(is16Bit)
        {
        }

    public:
        uint32_t indexCount;

    private:
        bool is16Bit_;
    };
}
