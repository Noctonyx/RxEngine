////////////////////////////////////////////////////////////////////////////////
// MIT License
//
// Copyright (c) 2021.  Shane Hyde (shane@noctonyx.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

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
            ImGui::Text(
                "(%.2f,%.2f,%.2f) %.2f",
                mesh->boundSphere.Center.x,
                mesh->boundSphere.Center.y,
                mesh->boundSphere.Center.z,
                mesh->boundSphere.Radius
            );

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("SubMeshes");
            ImGui::TableNextColumn();

            for (auto & sm : mesh->subMeshes) {
                ImGui::Text("%lld ", sm);
            }
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
        //rdc.boundSphere = mesh->boundSphere;

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
        world_->set<ComponentGui>(
            world_->getComponentId<MeshBundle>(),
            ComponentGui{.editor = meshBundleGui}
        );
        world_->set<ComponentGui>(
            world_->getComponentId<Mesh>(),
            ComponentGui{.editor = meshPrimitiveGui}
        );
        world_->set<ComponentGui>(
            world_->getComponentId<SubMesh>(),
            ComponentGui{.editor = subMeshGui}
        );

        world_->createSystem("Mesh:CacheSubmeshData")
              .inGroup("Pipeline:PreFrame")
              .withQuery<SubMesh>()
              .without<RenderDetailCache>()
              .each(cacheMeshRenderDetails);
    }

    void MeshModule::shutdown()
    {
        world_->remove<ComponentGui>(world_->getComponentId<MeshBundle>());
        world_->remove<ComponentGui>(world_->getComponentId<Mesh>());
        world_->remove<ComponentGui>(world_->getComponentId<SubMesh>());

        world_->lookup("Mesh:CacheSubmeshData").destroy();
    }

    void MeshModule::renderIndirectDraws(ecs::World * world,
        IndirectDrawSet ids,
        const std::shared_ptr<RxCore::SecondaryCommandBuffer> & buf)
    {
        OPTICK_EVENT()
        ecs::entity_t current_pipeline{};
        ecs::entity_t prevBundle = 0;

        for (auto & h: ids.headers) {
            OPTICK_EVENT("IDS Header")
            if (h.commandCount == 0) {
                continue;
            }
            {
                OPTICK_EVENT("Set Pipeline and buffers")
                if (h.pipelineId != current_pipeline) {

                    auto pl = world->get<GraphicsPipeline>(h.pipelineId);
                    buf->bindPipeline(pl->pipeline->Handle());
                    current_pipeline = h.pipelineId;
                }
                if (h.bundle != prevBundle) {

                    OPTICK_EVENT("Bind Bundle")
                    auto bund = world->get<MeshBundle>(h.bundle);
                    buf->pushConstant(
                        vk::ShaderStageFlagBits::eVertex, 0,
                        sizeof(bund->address), &bund->address
                    );
                    {
                        OPTICK_EVENT("Bind IB")
                        buf->bindIndexBuffer(bund->indexBuffer);
                    }
                    prevBundle = h.bundle;
                }
            }
            {
                OPTICK_EVENT("Draw Indexed")
                for (uint32_t i = 0; i < h.commandCount; i++) {
                    auto & c = ids.commands[i + h.commandStart];
                    buf->DrawIndexed(
                        c.indexCount, c.instanceCount, c.indexOffset, c.vertexOffset,
                        c.instanceOffset
                    );
                }
            }
        }
    }
}
