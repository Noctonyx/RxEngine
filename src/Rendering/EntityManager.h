//
// Created by shane on 8/03/2021.
//

#ifndef RX_ENTITYMANAGER_H
#define RX_ENTITYMANAGER_H

#include <cstdint>
#include <filesystem>
#include <vector>
#include "MeshBundle.h"
#include "MaterialManager.h"
#include "Defines.h"
#include "DirectXCollision.h"

namespace RxEngine
{
    struct EntityId
    {
        friend class EntityManager;

        EntityId() = default;

        explicit EntityId(uint32_t id)
            : id(id) {}

        [[nodiscard]] bool valid() const
        {
            return id != RX_INVALID_ID;
        }

        bool operator==(const EntityId & other) const
        {
            return id == other.id;
        }

    private:
        [[nodiscard]] uint32_t index() const
        {
            return id;
        }

        uint32_t id = RX_INVALID_ID;
    };

    struct Entity
    {
        std::shared_ptr<MeshBundle> bundle;
        uint8_t lodCount;
        std::array<std::pair<uint32_t, float>, 4> lods;
        MaterialId materialId;
        DirectX::BoundingBox bounds;
        // BoundingSphere sphereBounds;
    };

    struct EntityGroup
    {
        uint32_t bufferSize;
        uint32_t vertexSize;
        std::weak_ptr<MeshBundle> bundle;
        bool fixedFunction;
    };

    struct BundledMeshPrimitive {
        uint32_t firstIndex;
        uint32_t indexCount;
    };

    struct BundledMesh {
        std::shared_ptr<MeshBundle> bundle;
        uint32_t meshIndex;
        std::vector<BundledMeshPrimitive> primitives;
    };

    class EntityManager
    {
    public:
        EntityManager(MaterialManager * materialManager);
        ~EntityManager();

        std::pair<std::shared_ptr<MeshBundle>, uint32_t> addMeshToGroup(uint32_t groupId, const MeshToBundle & mesh);
        std::pair<std::shared_ptr<MeshBundle>, std::vector<uint32_t>> addMeshesToGroup(
            uint32_t groupId,
            const std::vector<MeshToBundle> & meshes);
        EntityId addEntity(const std::string & name, const Entity & entity);
        //uint32_t addEntity(const std::string & name, const Entity & entity);
        void removeEntity(EntityId entityId);

        const Entity & getEntity(EntityId ix) const
        {
            return entities_[ix.index()];
        };

        //void loadEntities(uint32_t groupId, std::vector<std::filesystem::path> loadList);
        EntityId loadEntity(uint32_t groupId, const std::filesystem::path & loadList);
        EntityId getIndexByName(const std::string & name) const;

        uint32_t addGroup(uint32_t groupBufferSize, size_t vertexSize, bool fixedFunction);

        MaterialPipelineId getEntityOpaquePipeline(EntityId entityId);
        MaterialPipelineId getEntityShadowPipeline(EntityId entityId);

        void ensureDescriptors(const RxCore::DescriptorPoolTemplate & poolTemplate,
                               vk::DescriptorSetLayout layout);

        void entityEditorGui();

    private:

        uint32_t lowestFreeIndex_{};

        MaterialManager * materialManager_;

        std::vector<EntityGroup> groups_{};
        std::vector<Entity> entities_{};

        std::unordered_map<std::string,BundledMesh> meshIndex_{};
        std::unordered_map<std::string, EntityId> entityIndex_{};
    };
}
#endif //RX_ENTITYMANAGER_H
