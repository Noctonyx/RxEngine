#include "EngineMain.hpp"
#include "imgui.h"

namespace RxEngine
{
    void EngineMain::ecsMainMenu(bool & show_entity_window,
                                 bool & show_systems_window,
                                 bool & show_components_window)
    {
        if (ImGui::BeginMenu("ECS")) {
            if (ImGui::MenuItem("Entities", nullptr, show_entity_window)) {
                show_entity_window = !show_entity_window;
                setBoolConfigValue("editor", "ecsEntityWindow", show_entity_window);
            }
            if (ImGui::MenuItem("Components", nullptr, show_components_window)) {
                show_components_window = !show_components_window;
                setBoolConfigValue("editor", "ecsComponentsWindow", show_components_window);
            }
            if (ImGui::MenuItem("Systems", nullptr, show_systems_window)) {
                show_systems_window = !show_systems_window;
                setBoolConfigValue("editor", "ecsSystemsWindow", show_systems_window);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }

    void EngineMain::ecsInspectorShowTableDetails(ecs::entity_t & selectedEntity,
                                                  ecs::WorldIterator it) const
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TableNextColumn();
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4) ImColor::HSV(0.5, 0.6f, 0.6f));

        ImGui::Button("Type");
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
        for (auto i: it) {
            auto e = it.entity(i);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%lld", e.id());
            ImGui::TableNextColumn();
            //ImGui::Text("%s", e.path().c_str());
            if (ImGui::Selectable(
                e.path().c_str(),
                selectedEntity == e)) {
                selectedEntity = e;
            }
        }
    }

    void EngineMain::ecsInspectorIsAEntity(ecs::entity_t entity, ecs::entity_t & selectedEntity)
    {
        entity.each(flecs::IsA, [&](const flecs::entity & object)
        {
            //ImU32 row_bg_color = ImGui::GetColorU32(ImVec4(0.7f, 0.3f, 0.3f, 0.65f)); // Flat or Gradient?
            //ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,  row_bg_color);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            bool open = ImGui::TreeNodeEx(object.name(), ImGuiTreeNodeFlags_SpanFullWidth,
                                          "IsA(%s)", object.name().c_str());
            ImGui::TableNextColumn();
            ImGui::TableNextColumn();
            ImGui::TableNextColumn();
            if (open) {
                ecsInspectorIsAEntity(object, selectedEntity);
                ecsInspectorEntityComponents(object, selectedEntity);
                ImGui::TreePop();
            }
        });
    }

    void EngineMain::ecsInspectorEntityComponents(ecs::entity_t entity,
                                                  ecs::entity_t & selectedEntity)
    {
        entity.each([&](flecs::entity & id)
        {
            if (id.has_relation(flecs::IsA)) {
                return;
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4) ImColor::HSV(0.7f, 0.6f, 0.6f));
            if (ImGui::Button(id.object().name())) {
                selectedEntity = id.object();
            }
            ImGui::PopStyleColor();
            ImGui::PopStyleVar(2);

            ImGui::TableNextColumn();
            if (id.has_role()) {
                ImGui::TextUnformatted(id.role_str());
            }

            ImGui::TableNextColumn();
            if (id.is_pair()) {
                ImGui::TextUnformatted(id.relation().name());
            }

            ImGui::TableNextColumn();
            if (id.has<ComponentGui>()) {
                id.get<ComponentGui>()->editor(entity);
            }
        });
    }

    void EngineMain::ecsInspectorEntityWindow(bool & show_entity_window)
    {
        bool is_open = show_entity_window;
        static ecs::entity_t selectedEntity;

        if (ImGui::Begin("Entities", &is_open)) {

            if (ImGui::BeginTable("Entities", 2,
                                  ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit |
                                  ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg,
                                  ImVec2(0, 400))) {
                ImGui::TableSetupColumn("ID");
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

                //world_->type()

                for (auto it: world) {
                    it.
                    
                    flecs::type table_type = it.table_type();
                    //table_index++;
                    //if (labels[selected_index] == table_type.str().c_str()) {
                    if (it.count() > 0) {
                        ecsInspectorShowTableDetails(selectedEntity, it, table_type);
                    }
                }
                ImGui::EndTable();
            }

            if (ImGui::BeginTable("FocusedEntity", 4,
                                  ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
                                  ImGuiTableFlags_Resizable | ImGuiWindowFlags_NoBackground |
                                  ImGuiTableFlags_NoBordersInBody)) {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Role", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Relation", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_NoHide);
                ImGui::TableHeadersRow();

                if (selectedEntity.is_valid()) {
                    auto entity = selectedEntity;

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::SetNextItemOpen(true, ImGuiCond_Once);

                    std::string type = "Entity";
                    if (entity.has<flecs::Component>()) {
                        type = "Component";
                    } else if (entity.has(EcsModule)) {
                        type = "Module";
                    } else if (entity.has(EcsPrefab)) {
                        type = "Prefab";
                    }

                    bool open = ImGui::TreeNodeEx(entity.name(), ImGuiTreeNodeFlags_SpanFullWidth,
                                                  "%s (%s) #%lld", type.c_str(),
                                                  entity.name().c_str(),
                                                  entity.id());
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();

                    if (open) {
                        ecsInspectorIsAEntity(entity, selectedEntity);
                        ecsInspectorEntityComponents(entity, selectedEntity);
                        ImGui::TreePop();
                    }
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();

        if (is_open != show_entity_window) {
            show_entity_window = is_open;
            >
            setBoolConfigValue("editor", "ecsEntityWindow", show_entity_window);
        }
    }

    void EngineMain::updateEntityGui()
    {
        static bool show_entity_window = getBoolConfigValue("editor", "ecsEntityWindow", false);
        static bool show_systems_window = getBoolConfigValue("editor", "ecsSystemsWindow", false);
        static bool show_components_window = getBoolConfigValue(
            "editor", "ecsComponentsWindow", false);

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Subsystems")) {
                ecsMainMenu(show_entity_window, show_systems_window, show_components_window);
            }
            ImGui::EndMainMenuBar();
        }

        if (show_entity_window) {
            ecsInspectorEntityWindow(show_entity_window);
        }
        if (show_components_window) {
            bool is_open = show_components_window;

            if (ImGui::Begin("Components", &is_open)) {}
            ImGui::End();

            if (is_open != show_components_window) {
                show_components_window = is_open;
                setBoolConfigValue("editor", "ecsComponentsWindow", show_components_window);
            }
        }
        if (show_systems_window) {
            bool is_open = show_systems_window;

            if (ImGui::Begin("Systems", &is_open)) {
                ecs_pipeline_stats_t s{nullptr, nullptr};
                //int32_t count = ecs_vector_count(s.systems);
                //s.systems = flecs::vector<>
                //flecs::vector<>
                ecs_get_pipeline_stats(world, ecs_get_pipeline(world.get_world()), &s);
                flecs::vector<ecs_entity_t> v(s.systems);
                //ecs_map_get()
                //flecs::map<>
                //int32_t i, count = ecs_vector_count(s.systems);
                //ecs_entity_t* systems = ecs_vector_first(s.systems, ecs_entity_t);
                //flecs::vector<>

                if (ImGui::BeginTable("Pipeline", 5,
                                      ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit |
                                      ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg,
                                      ImVec2(0, 400))) {
                    ImGui::TableSetupColumn("Name");
                    ImGui::TableSetupColumn("Active");
                    ImGui::TableSetupColumn("Tables");
                    ImGui::TableSetupColumn("Matched");
                    ImGui::TableSetupColumn("Time/Avg");
                    ImGui::TableSetupScrollFreeze(0, 1);
                    ImGui::TableHeadersRow();

                    for (auto x: v) {
                        if (x == 0) {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::Text("Merge");
                        } else {
                            ecs_system_stats_t * s1 = ecs_map_get(
                                s.system_stats, ecs_system_stats_t, x);
                            int32_t t = s1->query_stats.t;

                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            auto e = world.entity(x);
                            ImGui::Text("%s", e.name().c_str());
                            ImGui::TableNextColumn();
                            ImGui::Text("%s", s1->active.avg[t] > 0.9f ? "Active" : "Inactive");
                            ImGui::TableNextColumn();
                            ImGui::Text("%.1f", s1->query_stats.matched_table_count.avg[t]);
                            ImGui::TableNextColumn();
                            ImGui::Text("%.1f", s1->query_stats.matched_entity_count.avg[t]);
                            ImGui::TableNextColumn();
                            ImGui::Text("%.3f/%.3f", s1->time_spent.value[t] * 1.f,
                                        s1->time_spent.rate.avg[t] * 1.f);
                        }
                    }
                    ImGui::EndTable();
                }

            }
            ImGui::End();

            if (is_open != show_systems_window) {
                show_systems_window = is_open;
                setBoolConfigValue("editor", "ecsSystemsWindow", show_systems_window);
            }
        }
    }
}
