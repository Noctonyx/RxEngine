#include "Mesh.h"

#include "EngineMain.hpp"
#include "imgui.h"

namespace RxEngine
{
    void meshPrimitiveGui(ecs::World *, void * ptr)
    {
        auto mesh = static_cast<Mesh *>(ptr);

        if (mesh) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Vertex Offset");
            ImGui::TableNextColumn();
            ImGui::Text("%d", mesh->vertexOffset);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Index Offset");
            ImGui::TableNextColumn();
            ImGui::Text("%d", mesh->indexOffset);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Index Count");
            ImGui::TableNextColumn();
            ImGui::Text("%d", mesh->indexCount);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("SphereBounds");
            ImGui::TableNextColumn();
            ImGui::Text("(%.2f,%.2f,%.2f) %.2f",
                        mesh->boundSphere.Center.x,
                        mesh->boundSphere.Center.y,
                        mesh->boundSphere.Center.z,
                        mesh->boundSphere.Radius);
        }
    }

    void subMeshGui(ecs::World *, void * ptr)
    {
        auto sub_mesh = static_cast<SubMesh *>(ptr);

        if (sub_mesh) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Index Offset");
            ImGui::TableNextColumn();
            ImGui::Text("%d", sub_mesh->indexOffset);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Index Count");
            ImGui::TableNextColumn();
            ImGui::Text("%d", sub_mesh->indexCount);
        }
    }

    void meshBundleGui(ecs::World *, void * ptr)
    {
        auto mesh_bundle = static_cast<MeshBundle *>(ptr);

        if (mesh_bundle) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Vertex Size");
            ImGui::TableNextColumn();
            ImGui::Text("%d", mesh_bundle->vertexSize);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Vertex Count");
            ImGui::TableNextColumn();
            ImGui::Text("%d/%d", mesh_bundle->vertexCount, mesh_bundle->maxVertexCount);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Index Count");
            ImGui::TableNextColumn();
            ImGui::Text("%d/%d", mesh_bundle->indexCount, mesh_bundle->maxIndexCount);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Mesh Count");
            ImGui::TableNextColumn();
            ImGui::Text("%lld", mesh_bundle->entries.size());
        }
    }

    void cacheMeshRenderDetails(ecs::EntityHandle subMeshEntity)
    {
        RenderDetailCache rdc{};

        auto mesh_entity = subMeshEntity.getRelatedEntity<SubMeshOf>();
        if (!mesh_entity) {
            return;
        }

        auto mesh = mesh_entity.get<Mesh>();

        rdc.vertexOffset = mesh->vertexOffset;
        rdc.indexCount = mesh->indexCount;
        rdc.indexOffset = mesh->indexOffset;
        rdc.boundSphere = mesh->boundSphere;

        const auto bundle_entity = mesh_entity.getRelatedEntity<InBundle>();
        if (!bundle_entity) {
            return;
        }
        rdc.bundle = bundle_entity;

        auto material_entity = subMeshEntity.getRelatedEntity<UsesMaterial>();
        if (!material_entity) {
            return;
        }

        rdc.material = material_entity;

        rdc.opaquePipeline = material_entity.getRelatedEntity<HasOpaquePipeline>();
        rdc.shadowPipeline = material_entity.getRelatedEntity<HasShadowPipeline>();
        rdc.transparentPipeline = material_entity.getRelatedEntity<HasTransparentPipeline>();

        subMeshEntity.setDeferred(rdc);
    }

    void MeshModule::startup()
    {
        world_->set<ComponentGui>(world_->getComponentId<MeshBundle>(),
                                  ComponentGui{.editor = meshBundleGui});
        world_->set<ComponentGui>(world_->getComponentId<Mesh>(),
                                  ComponentGui{.editor = meshPrimitiveGui});
        world_->set<ComponentGui>(world_->getComponentId<SubMesh>(),
                                  ComponentGui{.editor = subMeshGui});

        world_->createSystem("Mesh:CacheSubmeshData")
              .inGroup("Pipeline:PreFrame")
              .withQuery<SubMesh>()
              .without<RenderDetailCache>()
              .each(cacheMeshRenderDetails);
    }

    void MeshModule::shutdown() {
        world_->remove<ComponentGui>(world_->getComponentId<MeshBundle>());
        world_->remove<ComponentGui>(world_->getComponentId<Mesh>());
        world_->remove<ComponentGui>(world_->getComponentId<SubMesh>());

        world_->lookup("Mesh:CacheSubmeshData").destroy();
    }
}
