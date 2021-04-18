//
// Created by shane on 8/03/2021.
//

#include "EntityManager.h"
#include "Defines.h"
#include <Loader.h>
#include <imgui.h>

namespace RxEngine
{
#if 0
    uint32_t EntityManager::addGroup(uint32_t groupBufferSize, size_t vertexSize, bool fixedFunction)
    {
        uint32_t new_ix = static_cast<uint32_t>(groups_.size());

        auto & g = groups_.emplace_back();
        g.bufferSize = groupBufferSize;
        g.vertexSize = static_cast<uint32_t>(vertexSize);
        g.fixedFunction = fixedFunction;

        return new_ix;
    }

    EntityId EntityManager::addEntity(const std::string & name, const Entity & entity)
    {
        if (lowestFreeIndex_ < entities_.size()) {
            uint32_t newIx = lowestFreeIndex_;
            entities_[lowestFreeIndex_] = entity;

            while (lowestFreeIndex_ < entities_.size()) {
                if (entities_[lowestFreeIndex_++].bundle == nullptr) {
                    break;
                }
            }
            if (name != "") {
                entityIndex_.emplace(name, newIx);
            }
            return EntityId{newIx};
        }

        auto newIx = static_cast<uint32_t>(entities_.size());
        entities_.push_back(entity);
        lowestFreeIndex_ = static_cast<uint32_t>(entities_.size());
        if (name != "") {
            entityIndex_.emplace(name, newIx);
        }
        return EntityId{newIx};
    }

    void EntityManager::removeEntity(EntityId entityId)
    {
        entities_[entityId.index()].bundle.reset();
        lowestFreeIndex_ = std::min(lowestFreeIndex_, entityId.index());
    }

    EntityManager::EntityManager(MaterialManager * materialManager)
        : materialManager_(materialManager) {}

    EntityManager::~EntityManager()
    {
        entities_.clear();
        groups_.clear();
    }

    std::pair<std::shared_ptr<MeshBundle>, uint32_t> EntityManager::addMeshToGroup(
        uint32_t groupId,
        const MeshToBundle & mesh)
    {
        std::shared_ptr<MeshBundle> current_bundle;

        auto & g = groups_[groupId];

        if (!g.bundle.expired()) {
            current_bundle = g.bundle.lock();
            auto x = current_bundle->add(mesh);

            if (x != RX_INVALID_ID) {
                return {current_bundle, x};
            }
            current_bundle = nullptr;
        }
        if (!current_bundle) {
            current_bundle = std::make_shared<RxEngine::MeshBundle>(
                g.vertexSize,
                g.bufferSize * 1024ULL * 1024ULL,
                !g.fixedFunction);
            g.bundle = current_bundle;
        }

        auto x = current_bundle->add(mesh);
        assert(x != RX_INVALID_ID);
        return {current_bundle, x};
    }

    EntityId EntityManager::getIndexByName(const std::string & name) const
    {
        if (entityIndex_.contains(name)) {
            return entityIndex_.at(name);
        }
        return {};
    }

    EntityId EntityManager::loadEntity(uint32_t groupId, const std::filesystem::path & path)
    {
        if (entityIndex_.contains(path.generic_string())) {
            return entityIndex_.at(path.generic_string());
        }

        auto ix = static_cast<uint32_t>(entities_.size());

        RxAssets::EntityData ed{};
        RxAssets::Loader::loadEntity(ed, path);

        auto & ep = entities_.emplace_back();
        ep.materialId = materialManager_->loadMaterial(ed.materialname);
        ep.lodCount = ed.lodCount;

        std::vector<MeshToBundle> m2bs;
        std::vector<RxAssets::MeshData> mds(ep.lodCount);

        std::vector<DirectX::XMFLOAT3> bps;

        for (uint32_t i = 0; i < ep.lodCount; i++) {
            ep.lods[i].second = ed.LODS[i].screenSpace;
            //RXAssets::MeshData md;
            RxAssets::Loader::loadMesh(mds[i], ed.LODS[i].modelName);

            auto & m2b = m2bs.emplace_back();

            m2b.indexCount = mds[i].indices.size();
            m2b.indices = mds[i].indices.data();
            m2b.vertexCount = mds[i].vertices.size();
            m2b.vertices = mds[i].vertices.data();
            bps.push_back(mds[i].minp);
            bps.push_back(mds[i].maxp);
            //ep.bounds.extend(mds[i].minp);
            //ep.bounds.extend(mds[i].maxp);
        }
        DirectX::BoundingBox::CreateFromPoints(ep.bounds, bps.size(), bps.data(), sizeof(DirectX::XMFLOAT3));

        auto[mb, mix] = addMeshesToGroup(groupId, m2bs);
        for (uint32_t i = 0; i < ep.lodCount; i++) {
            ep.lods[i].first = mix[i];
        }
        ep.bundle = mb;

        lowestFreeIndex_ = static_cast<uint32_t>(entities_.size());
        return EntityId{ix};
    }

    std::pair<std::shared_ptr<MeshBundle>, std::vector<uint32_t>> EntityManager::addMeshesToGroup(
        uint32_t groupId,
        const std::vector<MeshToBundle> & meshes)
    {
        std::shared_ptr<MeshBundle> current_bundle;

        auto & g = groups_[groupId];

        if (!g.bundle.expired()) {
            current_bundle = g.bundle.lock();

            if (current_bundle->canFit(meshes)) {
                std::vector<uint32_t> ids;
                for (auto & m :meshes) {
                    ids.push_back(current_bundle->add(m));
                }
                return {current_bundle, ids};
            }
            current_bundle = nullptr;
        }
        if (!current_bundle) {
            current_bundle = std::make_shared<RxEngine::MeshBundle>(
                g.vertexSize,
                g.bufferSize * 1024ULL * 1024ULL,
                !g.fixedFunction);
            g.bundle = current_bundle;
        }
        std::vector<uint32_t> ids;
        for (auto & m :meshes) {
            ids.push_back(current_bundle->add(m));
        }
        return {current_bundle, ids};
    }

    MaterialPipelineId EntityManager::getEntityOpaquePipeline(EntityId entityId)
    {
        auto & e = entities_[entityId.index()];

        auto & m = materialManager_->getMaterial(e.materialId);
        auto & mb = materialManager_->getMaterialBase(m.materialBaseId);
        return mb.opaquePipelineId;
    }

    MaterialPipelineId EntityManager::getEntityShadowPipeline(EntityId entityId)
    {
        auto & e = entities_[entityId.index()];

        auto & m = materialManager_->getMaterial(e.materialId);
        auto & mb = materialManager_->getMaterialBase(m.materialBaseId);
        return mb.shadowPipelineId;
    }
    void EntityManager::ensureDescriptors(
        const RxCore::DescriptorPoolTemplate & poolTemplate,
        vk::DescriptorSetLayout layout)
    {
        for(auto & g: groups_) {
            if(!g.bundle.expired()) {
                g.bundle.lock()->ensureDescriptorSet(poolTemplate, layout);
            }
        }
    }

    void EntityManager::entityEditorGui()
    {
        if (ImGui::BeginTabBar("EntityManager")) {

            if (ImGui::BeginTabItem("Entities")) {
                //materialEditor();

                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
#endif
}