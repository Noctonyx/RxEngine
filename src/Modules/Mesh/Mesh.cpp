#include "Mesh.h"

namespace RxEngine
{

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
        world_->createSystem("Mesh:CacheSubmeshData")
            .inGroup("Pipeline:PreFrame")
            .withQuery<SubMesh>()
            .without<RenderDetailCache>()
            .each(cacheMeshRenderDetails);
    }

    void MeshModule::shutdown() {}
}
