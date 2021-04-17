//
// Created by shane on 24/02/2021.
//

#ifndef RX_BATCHDATA_H
#define RX_BATCHDATA_H

namespace RxEngine
{
#if 0
    struct EntityLOD
    {
        uint32_t meshId;
        float distance;
    };

    struct EntityEntry
    {
        uint32_t materialId;
        uint8_t numLODs;
        std::array<EntityLOD, 4> LODs;
    };

    struct BatchEntry
    {
        Directx:: transform;
        glm::vec4 sphereBounds;
        uint32_t entityId;
    };
#endif
}

#endif //RX_BATCHDATA_H
