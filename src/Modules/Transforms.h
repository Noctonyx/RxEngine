#pragma once

#include "RxECS.h"
#include "imgui.h"
#include "DirectXCollision.h"
#include "SerialisationData.h"

struct Sector;

namespace RxEngine
{
    struct Transforms
    {
        struct WorldPosition
        {
            DirectX::XMFLOAT3 position;
        };

        struct YRotation
        {
            float yRotation;
        };

        struct ScalarScale
        {
            float scale;
        };

        struct BoundingSphere
        {
            DirectX::BoundingSphere boundSphere;
        };

        struct BoundingBox
        {
            DirectX::BoundingBox boundBox;
        };

        static void registerModule(ecs::World * world)
        {
            const auto worldPosition = world->getComponentId<WorldPosition>();
            const auto yrot = world->getComponentId<YRotation>();
            const auto scale = world->getComponentId<ScalarScale>();

            world->set<RxEngine::ComponentGui>(
                worldPosition,
                {
                    .editor = [](ecs::World * w, ecs::entity_t e)
                    {
                        auto position = w->get<WorldPosition>(e);
                        if (position) {
                            if (ImGui::BeginTable("ComponentGui", 2, /*ImGuiTableFlags_Borders | */
                                                  ImGuiTableFlags_Resizable |
                                                  ImGuiTableFlags_Hideable)) {
                                ImGui::TableSetupColumn("Name");
                                ImGui::TableSetupColumn("Value");
                                //ImGui::TableHeadersRow();

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

                                ImGui::EndTable();
                            }
                        }
                    }
                });

            world->set<RxEngine::ComponentGui>(
                yrot,
                {
                    .editor = [](ecs::World * w, ecs::entity_t e)
                    {
                        auto rotation = w->get<YRotation>(e);
                        if (rotation) {
                            if (ImGui::BeginTable("ComponentGui", 2, /*ImGuiTableFlags_Borders | */
                                                  ImGuiTableFlags_Resizable |
                                                  ImGuiTableFlags_Hideable)) {
                                ImGui::TableSetupColumn("Name");
                                ImGui::TableSetupColumn("Value");
                                //ImGui::TableHeadersRow();

                                ImGui::TableNextRow();
                                ImGui::TableNextColumn();
                                ImGui::Text("Y Rotation");
                                ImGui::TableNextColumn();
                                ImGui::Text("%.1f", rotation->yRotation);

                                ImGui::EndTable();
                            }
                        }
                    }

                });
            world->set<RxEngine::ComponentGui>(
                scale,
                {
                    .editor = [](ecs::World * w, ecs::entity_t e)
                    {
                        auto sc = w->get<ScalarScale>(e);
                        if (sc) {
                            if (ImGui::BeginTable("ComponentGui", 2, /*ImGuiTableFlags_Borders | */
                                                  ImGuiTableFlags_Resizable |
                                                  ImGuiTableFlags_Hideable)) {
                                ImGui::TableSetupColumn("Name");
                                ImGui::TableSetupColumn("Value");
                                //ImGui::TableHeadersRow();

                                ImGui::TableNextRow();
                                ImGui::TableNextColumn();
                                ImGui::Text("Scale");
                                ImGui::TableNextColumn();
                                ImGui::Text("%.1f", sc->scale);

                                ImGui::EndTable();
                            }
                        }
                    }
                }
            );
        };
    };
};
