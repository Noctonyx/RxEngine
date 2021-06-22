//
// Created by shane on 24/03/2020.
//
#include "VertexBuffer.h"

namespace RxCore
{
    VertexBuffer::VertexBuffer(
        Device * device, VkBuffer handle,
        std::shared_ptr<Allocation> allocation,
        uint32_t vertexCount,
        uint32_t vertexSize)
        : Buffer(device, handle, std::move(allocation), vertexCount * vertexSize)
        , vertexSize(vertexSize)
        , vertexCount(vertexCount)
    {
    }
} // namespace RXCore