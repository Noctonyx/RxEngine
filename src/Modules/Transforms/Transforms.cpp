#include "Transforms.h"
#include "imgui.h"

namespace RxEngine
{
    using namespace Transforms;

    void TransformsModule::registerModule()
    {
        world_->set<ComponentGui>(world_->getComponentId<WorldPosition>(),
                                  {.editor = worldPositionGui});
        world_->set<ComponentGui>(world_->getComponentId<XRotation>(),
                                  {.editor = xRotationGui});
        world_->set<ComponentGui>(world_->getComponentId<YRotation>(),
                                  {.editor = yRotationGui});
        world_->set<ComponentGui>(world_->getComponentId<ScalarScale>(),
                                  {.editor = scalarScaleGui});
    }

    void TransformsModule::deregisterModule()
    {
        world_->remove<ComponentGui>(world_->getComponentId<WorldPosition>());
        world_->remove<ComponentGui>(world_->getComponentId<XRotation>());
        world_->remove<ComponentGui>(world_->getComponentId<YRotation>());
        world_->remove<ComponentGui>(world_->getComponentId<ScalarScale>());
    }

    void TransformsModule::worldPositionGui(ecs::EntityHandle e)
    {
        auto position = e.get<WorldPosition>();

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

    void TransformsModule::yRotationGui(ecs::EntityHandle e)
    {
        auto rotation = e.get<YRotation>();
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

    void TransformsModule::xRotationGui(ecs::EntityHandle e)
    {
        auto rotation = e.get<XRotation>();
        if (rotation) {
            if (ImGui::BeginTable("ComponentGui", 2, /*ImGuiTableFlags_Borders | */
                                  ImGuiTableFlags_Resizable |
                                  ImGuiTableFlags_Hideable)) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Value");
                //ImGui::TableHeadersRow();

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("X Rotation");
                ImGui::TableNextColumn();
                ImGui::Text("%.1f", rotation->xRotation);

                ImGui::EndTable();
            }
        }
    }

    void TransformsModule::scalarScaleGui(ecs::EntityHandle e)
    {
        auto sc = e.get<ScalarScale>();

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
