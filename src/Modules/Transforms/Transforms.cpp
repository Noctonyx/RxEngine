#include "Transforms.h"
#include "imgui.h"

namespace RxEngine
{
    using namespace Transforms;

    void worldPositionGui(ecs::World *, void * ptr)
    {
        auto position = static_cast<WorldPosition *>(ptr);

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

    void scalarScaleGui(ecs::World *, void * ptr)
    {
        auto sc = static_cast<ScalarScale *>(ptr);

        if (sc) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Scale");
            ImGui::TableNextColumn();
            ImGui::Text("%.1f", sc->scale);
        }
    }

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
}
