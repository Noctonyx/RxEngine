#include "Transforms.h"
#include "imgui.h"

namespace RxEngine
{
#if 0
    using namespace Transforms;

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

    void localRotationGui(ecs::EntityHandle, const void* ptr)
    {
        auto rotation = static_cast<const LocalRotation*>(ptr);
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
        auto sc = static_cast<const ScalarScale *>(ptr);

        if (sc) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Scale");
            ImGui::TableNextColumn();
            ImGui::Text("%.1f", sc->scale);
        }
    }

    void TransformsModule::startup()
    {
        world_->set<ComponentGui>(world_->getComponentId<WorldPosition>(),
                                  {.editor = worldPositionGui});
        world_->set<ComponentGui>(world_->getComponentId<LocalRotation>(),
                                  {.editor = localRotationGui});
        world_->set<ComponentGui>(world_->getComponentId<ScalarScale>(),
                                  {.editor = scalarScaleGui});
    }

    void TransformsModule::shutdown()
    {
        world_->remove<ComponentGui>(world_->getComponentId<WorldPosition>());
        world_->remove<ComponentGui>(world_->getComponentId<LocalRotation>());
        world_->remove<ComponentGui>(world_->getComponentId<ScalarScale>());
    }
#endif
}
