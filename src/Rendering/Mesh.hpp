//
// Created by shane on 30/12/2020.
//

#ifndef RX_MESH_HPP
#define RX_MESH_HPP

#include <RXCore.h>

#include "DirectXCollision.h"
#include "MeshBundle.h"

namespace RxEngine
{
    //class Material;
    struct SubMesh
    {
        uint32_t firstIndex;
        uint32_t indexCount;
        uint32_t materialIndex;
    };

    class Mesh //: public IAsset
    {
        //friend class AssetCache;

    public:
        Mesh(std::shared_ptr<MeshBundle> bundle, uint32_t meshId);
        ~Mesh();

        std::shared_ptr<MeshBundle> bundle() const { return meshBundle_; }
        //void Load(const char * meshName);
        //bool IsLoaded() const;
#if 0
        //void SetVertexBuffer(uint32_t vertexSize, uint32_t vertexCount, void * data);
        //void SetIndexBuffer(uint32_t indexCount, void * data);
        void SetBoundingBox(AABB & boundingBox)
        {
            boundingBox_ = boundingBox;
        }

        void SetBoundingBox(const glm::vec3 & minp, const glm::vec3 & maxp)
        {
            boundingBox_ = AABB(minp, maxp);
        }
#endif
        uint32_t getMeshId() const;

    public:
        //[[nodiscard]] const std::shared_ptr<RXCore::VertexBuffer> GetVertexBuffer() const;
        //[[nodiscard]] const std::shared_ptr<RXCore::IndexBuffer> GetIndexBuffer() const;

        //[[nodiscard]] uint32_t GetSubMeshCount() const { return static_cast<uint32_t>(subMeshes.size());}
        //[[nodiscard]] SubMesh GetSubMesh(uint32_t ix) const { return subMeshes[ix];}

        DirectX::BoundingBox getBounds() const
        {
            return boundingBox_;
        }
    private:
        uint32_t meshId;
        //std::vector<SubMesh> subMeshes;
        std::shared_ptr<MeshBundle> meshBundle_;
        //std::shared_ptr<RXCore::VertexBuffer> vb_;
        //std::shared_ptr<RXCore::IndexBuffer> ib_;
        //std::vector<std::string> materialNames;
        //std::vector<std::shared_ptr<Material>> materials;
        DirectX::BoundingBox boundingBox_;
        //uint32_t meshId;
        //bool isLoaded_ = false;
    };
}
#endif //RX_MESH_HPP
