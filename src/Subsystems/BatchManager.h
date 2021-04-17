//
// Created by shane on 24/02/2021.
//

#ifndef RX_BATCHMANAGER_H
#define RX_BATCHMANAGER_H

#include <cstdint>
#include <vector>
#include <array>
//#include <glm/fwd.hpp>
//#include <glm/mat4x4.hpp>
#include "Rendering/BatchData.h"

namespace RxEngine
{
#if 0
    struct BatchManagerEntry {

    };

    class BatchManager
    {
    public:
        BatchManager();

        void removeBatch(uint32_t batchId);
        void replaceBatch(uint32_t batchId, const std::vector<BatchEntry> & batchEntries);
        uint32_t addBatch(const std::vector<BatchEntry> & batchEntries);

    private:
        std::vector<BatchManagerEntry> batchEntries_;
        uint32_t nextBatchId;
    };
#endif
}
#endif //RX_BATCHMANAGER_H
