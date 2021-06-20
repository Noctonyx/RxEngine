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

//
// Created by shane on 20/06/2021.
//

#include <EngineMain.hpp>
#include <imgui.h>
#include "SceneModule.h"

using namespace DirectX;

namespace RxEngine
{

    void worldPositionGui(ecs::EntityHandle, const void * ptr)
    {
        auto position = static_cast<const WorldPosition *>(ptr);

        if (position) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("X");
            ImGui::TableNextColumn();
            ImGui::Text("%.3f", position->position.x);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Y");
            ImGui::TableNextColumn();
            ImGui::Text("%.3f", position->position.y);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Z");
            ImGui::TableNextColumn();
            ImGui::Text("%.3f", position->position.z);
        }
    }

    void localPositionGui(ecs::EntityHandle, const void * ptr)
    {
        auto position = static_cast<const LocalPosition *>(ptr);

        if (position) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("X");
            ImGui::TableNextColumn();
            ImGui::Text("%.3f", position->position.x);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Y");
            ImGui::TableNextColumn();
            ImGui::Text("%.3f", position->position.y);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Z");
            ImGui::TableNextColumn();
            ImGui::Text("%.3f", position->position.z);
        }
    }

    void localRotationGui(ecs::EntityHandle, const void * ptr)
    {
        auto rotation = static_cast<const LocalRotation *>(ptr);
        if (rotation) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Rotation");
            ImGui::TableNextColumn();
            ImGui::Text("%.1f, %.1f, %.1f", rotation->rotation.x, rotation->rotation.y, rotation->rotation.z);
        }
    }

#if 0
    void yRotationGui(ecs::World *, void * ptr)
    {
            auto rotation = static_cast<YRotation *>(ptr);
            if (rotation) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Y Rotation");
                ImGui::TableNextColumn();
                ImGui::Text("%.1f", rotation->yRotation);
            }
    }

    void xRotationGui(ecs::World *, void * ptr)
    {
            auto rotation = static_cast<XRotation *>(ptr);

            if (rotation) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("X Rotation");
                ImGui::TableNextColumn();
                ImGui::Text("%.1f", rotation->xRotation);
            }
    }
#endif

    void scalarScaleGui(ecs::EntityHandle, const void * ptr)
    {
        auto sc = static_cast<const LocalScale *>(ptr);

        if (sc) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Scale");
            ImGui::TableNextColumn();
            ImGui::Text("%.1f", sc->scale);
        }
    }

    SceneModule::SceneModule(ecs::World * world, EngineMain * engine, const ecs::entity_t moduleId)
        : Module(world, engine, moduleId)
    {}

    void SceneModule::startup()
    {
        updateTransformQueue = world_->createEntityQueue("UpdateWorldTransform");
        auto eq2 = world_->createEntityQueue("UpdateSceneNode");
        eq2.triggerOnAdd<LocalPosition>();
        eq2.triggerOnAdd<LocalRotation>();
        eq2.triggerOnUpdate<LocalPosition>();
        eq2.triggerOnUpdate<LocalRotation>();
        eq2.triggerOnUpdate<SceneNode>();

        world_->createSystem("Scene:MarkChildrenForUpdate")
              .inGroup("Pipeline:PostUpdate")
              .withEntityQueue(eq2)
              .withWrite<SceneNode>()
              .eachEntity([this](ecs::EntityHandle e) { return updatedSceneNode(e); });

        world_->createSystem("WorldObject:UpdateWorldTransform")
              .inGroup("Pipeline:PostUpdate")
              .withEntityQueue(updateTransformQueue)
              .withRead<LocalRotation>()
              .withRead<LocalPosition>()
              .withRead<SceneNode>()
              .withWrite<WorldTransform>()
              .withWrite<WorldPosition>()
              .withWrite<WorldBoundingSphere>()
              .eachEntity(&updateTransforms);

        world_->set<ComponentGui>(
            world_->getComponentId<WorldPosition>(),
            {.editor = worldPositionGui}
        );
        world_->set<ComponentGui>(
            world_->getComponentId<LocalPosition>(),
            {.editor = localPositionGui}
            );
        world_->set<ComponentGui>(
            world_->getComponentId<LocalRotation>(),
            {.editor = localRotationGui}
        );
        world_->set<ComponentGui>(
            world_->getComponentId<LocalScale>(),
            {.editor = scalarScaleGui}
        );
    }

    void SceneModule::shutdown()
    {
        world_->lookup("UpdateWorldTransform").destroy();
        world_->deleteSystem(world_->lookup("WorldObject:UpdateWorldTransform"));

        world_->remove<ComponentGui>(world_->getComponentId<WorldPosition>());
        world_->remove<ComponentGui>(world_->getComponentId<LocalPosition>());
        world_->remove<ComponentGui>(world_->getComponentId<LocalRotation>());
        world_->remove<ComponentGui>(world_->getComponentId<LocalScale>());
    }

    bool SceneModule::updatedSceneNode(ecs::EntityHandle e)
    {
        //auto sn = e.get<SceneNode>();
        std::deque<ecs::entity_t> toUpdate;
        toUpdate.push_back(e);
        /*
        for (auto c: sn->children) {
            toUpdate.push_back(c);
        }
        updateTransformQueue.post(e);
*/
        while (!toUpdate.empty()) {
            auto ce = e.getHandle(toUpdate.front());
            auto sn = ce.get<SceneNode>();
            if (sn) {
                for (auto c: sn->children) {
                    toUpdate.push_back(c);
                }
            }
            updateTransformQueue.post(ce);
            toUpdate.pop_front();
        }

        return true;
    }

    bool SceneModule::updateTransforms(ecs::EntityHandle e)
    {
        auto sn = e.get<SceneNode>();
        auto lp = e.get<LocalPosition>();
        auto rot = e.get<LocalRotation>();
        auto bb = e.get<LocalBoundingBox>();
        if (!sn || !lp || !bb) {
            return true;
        }
        auto parent = e.getHandle(sn->parent);

        XMMATRIX pm = XMMatrixIdentity();

        if (parent.isAlive()) {
            auto wt = parent.get<WorldTransform>();
            if (wt) {
                pm = XMLoadFloat4x4(&wt->transform);
            }
        }

        auto tm = XMMatrixTranslation(lp->position.x, lp->position.y, lp->position.z);
        auto rm = XMMatrixRotationRollPitchYaw(
            rot ? rot->rotation.x : 0.0f, rot ? rot->rotation.y : 0.0f,
            rot ? rot->rotation.z : 0.0f
        );
        auto nm = XMMatrixMultiply(rm, tm);
        nm = XMMatrixMultiply(pm, tm);

        BoundingSphere bs;
        BoundingSphere::CreateFromBoundingBox(bs, bb->boundBox);

        e.add<WorldTransform>();
        e.update<WorldTransform>(
            [&](WorldTransform * wt) {
                XMStoreFloat4x4(&wt->transform, nm);
            }
        );

        e.add<WorldBoundingSphere>();
        e.update<WorldBoundingSphere>(
            [&](WorldBoundingSphere * bs2) {
                bs.Transform(bs2->boundSphere, nm);
            }
        );

        XMFLOAT4 origin = {0, 0, 0, 1};

        e.add<WorldPosition>();
        e.update<WorldPosition>(
            [&](WorldPosition * wp) {
                auto p = XMLoadFloat4(&origin);
                auto wpp = XMVector3TransformCoord(p, nm);
                XMStoreFloat3(&wp->position, wpp);
            }
        );
        return true;
    }
}