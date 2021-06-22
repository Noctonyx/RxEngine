//
// Created by shane on 8/02/2021.
//

#include "Allocation.h"

namespace RxCore
{
    Allocation::Allocation(VmaAllocator allocator, VmaAllocation allocation)
        : allocation_(allocation)
        , allocator_(allocator)
    {

    }

    Allocation::~Allocation()
    {
        unmap();
        vmaFreeMemory(allocator_, allocation_);
    }
}