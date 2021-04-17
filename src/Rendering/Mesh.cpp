//
// Created by shane on 30/12/2020.
//

#include <filesystem>
#include "Mesh.hpp"
//#include "tiny_obj_loader.h"
//#include "meshoptimizer.h"

namespace RxEngine
{
    Mesh::~Mesh()
    {
        meshBundle_.reset();
    }

    Mesh::Mesh(std::shared_ptr<MeshBundle> bundle, uint32_t meshId)
        : meshId(meshId)
        , meshBundle_(std::move(bundle))
    {
    }

    uint32_t Mesh::getMeshId() const
    {
        return meshId;
    }
}
