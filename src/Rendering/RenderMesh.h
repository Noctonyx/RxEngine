#pragma once
#include <RXCore.h>

namespace RxEngine
{
  //  class Mesh;
//    class Material;

    struct LodDetails
    {
        float distance;
        uint32_t meshId;
    };

    struct RenderMesh
    {
        std::vector<LodDetails> meshDetails;
        glm::mat4 transform;
        uint32_t materialId;
    };
}
