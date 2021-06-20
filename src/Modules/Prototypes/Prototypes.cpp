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

#include <Modules/Scene/SceneModule.h>
#include "Prototypes.h"

#include "imgui.h"
#include "Modules/Mesh/Mesh.h"
#include "Modules/StaticMesh/StaticMesh.h"
#include "Modules/Transforms/Transforms.h"
#include "Modules/WorldObject/WorldObject.h"
#include "sol/table.hpp"

namespace RxEngine
{
    void visiblePrototypeUi(ecs::EntityHandle e, const void * ptr)
    {
        ecs::World * world = e.getWorld();

        auto visible_prototype = static_cast<const VisiblePrototype *>(ptr);

        if (visible_prototype) {
            for (auto x: visible_prototype->subMeshEntities) {
                auto sm = world->get<SubMesh>(x);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Mesh Entry");
                ImGui::TableNextColumn();
                ImGui::Text("%d", sm->indexCount);
            }
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Bounding Box");
            ImGui::TableNextColumn();
            ImGui::Text(
                "(%.2f,%.2f,%.2f) (%.2f,%.2f,%.2f)",
                visible_prototype->boundingBox.Center.x,
                visible_prototype->boundingBox.Center.y,
                visible_prototype->boundingBox.Center.z,
                visible_prototype->boundingBox.Extents.x,
                visible_prototype->boundingBox.Extents.y,
                visible_prototype->boundingBox.Extents.z
            );
        }
    }

    void PrototypesModule::startup()
    {
        world_->set<ComponentGui>(world_->getComponentId<VisiblePrototype>(),
                                  {.editor = visiblePrototypeUi});
    }

    void PrototypesModule::shutdown() { }

    void loadPrototype(ecs::World * world,
                       const std::string & prototypeName,
                       const sol::table & details)
    {
        auto e = world->newEntity(prototypeName.c_str()).add<ecs::Prefab>();
        if (details.get_or("world_position", false)) {
            e.add<WorldPosition>();
        }
        if (details.get_or("rotation", false)) {
            e.add<LocalRotation>();
        }
        if (details.get_or("world_object", false)) {
            e.add<SceneNode>();
        }
        if (details.get_or("scene_node", false)) {
            e.add<SceneNode>();
        }

        std::string visible = details.get<std::string>("visible");
        auto visible_entity = world->lookup(visible.c_str());
        assert(visible_entity.isAlive());
        auto vpd = visible_entity.get<VisiblePrototype>();
        e.set<HasVisiblePrototype>({{visible_entity}});
        e.set<LocalBoundingBox>({vpd->boundingBox});
    }

    void loadPrototypes(ecs::World * world, sol::table & prototypes)
    {
        for (auto & [key, value]: prototypes) {
            const std::string name = key.as<std::string>();
            sol::table details = value;
            loadPrototype(world, name, details);
        }
    }

    void loadVisible(ecs::World * world,
                     const std::string & visibleName,
                     const sol::table & details)
    {
        auto e = world->newEntity(visibleName.c_str());

        e.addAndUpdate<VisiblePrototype>([&](VisiblePrototype * vp){
            sol::table objects = details.get<sol::table>("objects");

            for (auto & [k, v]: objects) {
                sol::table objectDetails = v.as<sol::table>();
                std::string m = objectDetails.get<std::string>("mesh");
                uint32_t smi = objectDetails.get<uint32_t>("submesh_id");

                auto meshEntity = world->lookup(m).get<Mesh>();
                vp->boundingBox = meshEntity->boundBox;
                assert(meshEntity);
                assert(meshEntity->subMeshes.size() > smi);

                auto se = e.getHandle(meshEntity->subMeshes[smi]);
                vp->subMeshEntities.push_back(se);
            }
        });
    }

    void loadVisibles(ecs::World * world, sol::table & visibles)
    {
        for (auto & [key, value]: visibles) {
            const std::string name = key.as<std::string>();
            sol::table details = value;
            loadVisible(world, name, details);
        }
    }

    void PrototypesModule::loadData(sol::table data)
    {
        sol::optional<sol::table> prototypes = data["prototype"];
        sol::optional<sol::table> visibles = data["visible"];

        if (visibles.has_value()) {
            loadVisibles(world_, visibles.value());
        }
        if (prototypes.has_value()) {
            loadPrototypes(world_, prototypes.value());
        }
    }
}
