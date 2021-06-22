#pragma once

#include "Buffer.hpp"

namespace RxCore
{
    class VertexBuffer : public Buffer
    {
    public:
        VertexBuffer(
            Device * device, VkBuffer handle,
            std::shared_ptr<Allocation> allocation,
            uint32_t vertexCount,
            uint32_t vertexSize
        );

        RX_NO_COPY_NO_MOVE(VertexBuffer);

        uint32_t vertexSize;
        uint32_t vertexCount;
    };
}
