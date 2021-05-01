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

    void EngineMain::ecsInspectorShowTableDetails(ecs::EntityHandle & selectedEntity,
                                                  ecs::Table * table) const
    {
        if (table->entities.size() == 0) {
            return;
        }
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TableNextColumn();
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4) ImColor::HSV(0.5, 0.6f, 0.6f));

        ImGui::Button(table->description().c_str());
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
        for (auto e: *table) {

            ImGui::PushID(e.id);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%lld", e.id);
            ImGui::TableNextColumn();
            //ImGui::Text("%s", e.path().c_str());
            if (ImGui::Selectable(
                e.description().c_str(),
                selectedEntity == e)) {
                selectedEntity = e;
            }
            ImGui::PopID();
        }
    }

    void EngineMain::ecsInspectorIsAEntity(ecs::EntityHandle entity,
                                           ecs::EntityHandle & selectedEntity)
    {
        if (entity.has<ecs::InstanceOf>()) {
            auto p = entity.get<ecs::InstanceOf>();
            auto object = entity.getHandle(p->entity);

            //ImU32 row_bg_color = ImGui::GetColorU32(ImVec4(0.7f, 0.3f, 0.3f, 0.65f)); // Flat or Gradient?
            //ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,  row_bg_color);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            bool open = ImGui::TreeNodeEx(object.description().c_str(),
                                          ImGuiTreeNodeFlags_SpanFullWidth,
                                          "IsA(%s)", object.description().c_str());
            ImGui::TableNextColumn();
            ImGui::TableNextColumn();
            ImGui::TableNextColumn();
            if (open) {
                ecsInspectorIsAEntity(object, selectedEntity);
                ecsInspectorEntityComponents(object, selectedEntity);
                ImGui::TreePop();
            }
        }
    }

    void EngineMain::ecsInspectorEntityComponents(ecs::EntityHandle entity,
                                                  ecs::EntityHandle & selectedEntity)
    {
        for (auto c: entity) {
            auto cd = world->getComponentDetails(c);

            if (!cd->isRelation) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.7f, 0.6f, 0.6f));
                if (ImGui::Button(cd->name.c_str())) {
                    //selectedEntity = id.object();
                }
                ImGui::PopStyleColor();
                ImGui::PopStyleVar(2);

                //            ImGui::TableNextColumn();
                //            if (id.has_role()) {
                //              ImGui::TextUnformatted(id.role_str());
                //        }

                //            ImGui::TableNextColumn();
                //            if (id.is_pair()) {
                //              ImGui::TextUnformatted(id.relation().name());
                //        }

                ImGui::TableNextColumn();
                if (world->has<ComponentGui>(c)) {
                    world->get<ComponentGui>(c)->editor(entity);
                }
            } else {
                auto r = reinterpret_cast<const ecs::Relation *>(entity.getWorld()->get(entity.id, c));
                auto object = entity.getHandle(r->entity);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                bool open = ImGui::TreeNodeEx(object.description().c_str(),
                    ImGuiTreeNodeFlags_SpanFullWidth,
                    "%s(%s)", cd->name.c_str(), object.description().c_str());
                ImGui::TableNextColumn();
                if (world->has<ComponentGui>(c)) {
                    world->get<ComponentGui>(c)->editor(entity);
                }
                else {
//                    ImGui::TableNextColumn();
  //                  ImGui::TableNextColumn();
                }
                if (open) {
                    //ecsInspectorIsAEntity(object, selectedEntity);
                    ecsInspectorEntityComponents(object, selectedEntity);
                    ImGui::TreePop();
                }
            }
        }
    }

    void EngineMain::ecsInspectorEntityWindow(bool & show_entity_window)
    {
        bool is_open = show_entity_window;
        static ecs::EntityHandle selectedEntity;

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

                for (auto it: *world) {


                    //table_index++;
                    //if (labels[selected_index] == table_type.str().c_str()) {
                    //                    if (it.count() > 0) {
                    ecsInspectorShowTableDetails(selectedEntity,
                                                 world->getTableForArchetype(it.id));
                    //                  }
                }
                ImGui::EndTable();
            }

            if (ImGui::BeginTable("FocusedEntity", 2,
                                  ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
                                  ImGuiTableFlags_Resizable | ImGuiWindowFlags_NoBackground |
                                  ImGuiTableFlags_NoBordersInBody)) {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
                //                ImGui::TableSetupColumn("Role", ImGuiTableColumnFlags_NoHide);
                //ImGui::TableSetupColumn("Relation", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_NoHide);
                ImGui::TableHeadersRow();

                if (selectedEntity.isAlive()) {
                    auto entity = selectedEntity;

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::SetNextItemOpen(true, ImGuiCond_Once);

                    std::string type = "Entity";
                    if (entity.has<ecs::Component>()) {
                        type = "Component";
                    } else if (entity.has<ecs::Prefab>()) {
                        type = "Prefab";
                    }

                    bool open = ImGui::TreeNodeEx(entity.description().c_str(),
                                                  ImGuiTreeNodeFlags_SpanFullWidth,
                                                  "%s (%s) %lld", type.c_str(),
                                                  entity.description().c_str(),
                                                  entity.id);
                    ImGui::TableNextColumn();
                    //                    ImGui::TableNextColumn();
                    //                  ImGui::TableNextColumn();

                    if (open) {
                        //ecsInspectorIsAEntity(entity, selectedEntity);
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
#if 0
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
#endif
            }
            ImGui::End();

            if (is_open != show_systems_window) {
                show_systems_window = is_open;
                setBoolConfigValue("editor", "ecsSystemsWindow", show_systems_window);
            }
        }
    }

    void EngineMain::ecsNameGui(ecs::EntityHandle e)
    {
        auto name = e.get<ecs::Name>();
        if (name) {
            if (ImGui::BeginTable("ComponentGui", 2, /*ImGuiTableFlags_Borders | */
                                  ImGuiTableFlags_Resizable |
                                  ImGuiTableFlags_Hideable)) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Value");
                //ImGui::TableHeadersRow();

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Name");
                ImGui::TableNextColumn();
                ImGui::Text("%s", name->name.c_str());

                ImGui::EndTable();
            }
        }
    }

    void EngineMain::ecsComponentGui(ecs::EntityHandle e)
    {
        auto comp = e.get<ecs::Component>();
        if (comp) {
            if (ImGui::BeginTable("ComponentGui", 2, /*ImGuiTableFlags_Borders | */
                ImGuiTableFlags_Resizable |
                ImGuiTableFlags_Hideable)) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Value");
                //ImGui::TableHeadersRow();

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Name");
                ImGui::TableNextColumn();
                ImGui::Text("%s", comp->name.c_str());

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Is Relation");
                ImGui::TableNextColumn();
                ImGui::Text("%s", comp->isRelation ? "True" : "False");

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Size");
                ImGui::TableNextColumn();
                ImGui::Text("%d", comp->size);

                ImGui::EndTable();
            }
        }
    }

    void EngineMain::ecsSystemGroupGui(ecs::EntityHandle e)
    {
        auto group = e.get<ecs::SystemGroup>();
        if (group) {
            if (ImGui::BeginTable("ComponentGui", 2, /*ImGuiTableFlags_Borders | */
                                  ImGuiTableFlags_Resizable |
                                  ImGuiTableFlags_Hideable)) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Value");
                //ImGui::TableHeadersRow();

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Sequence");
                ImGui::TableNextColumn();
                ImGui::Text("%d", group->sequence);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Fixed");
                ImGui::TableNextColumn();
                ImGui::Text("%s", group->fixed ? "True" : "False");

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Delta");
                ImGui::TableNextColumn();
                ImGui::Text("%.3f", group->delta);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Rate");
                ImGui::TableNextColumn();
                ImGui::Text("%.3f", group->rate);

                ImGui::EndTable();
            }
        }
    }

    void EngineMain::ecsWindowDetailsGui(ecs::EntityHandle e)
    {
        auto comp = e.get<WindowDetails>();
        if (comp) {
            if (ImGui::BeginTable("ComponentGui", 2, /*ImGuiTableFlags_Borders | */
                                  ImGuiTableFlags_Resizable |
                                  ImGuiTableFlags_Hideable)) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Value");
                //ImGui::TableHeadersRow();

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Width");
                ImGui::TableNextColumn();
                ImGui::Text("%d", comp->width);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Height");
                ImGui::TableNextColumn();
                ImGui::Text("%d", comp->height);

                ImGui::EndTable();
            }
        }
    }

    void EngineMain::ecsEngineTimeGui(ecs::EntityHandle e)
    {
        auto comp = e.get<EngineTime>();
        if (comp) {
            if (ImGui::BeginTable("ComponentGui", 2, /*ImGuiTableFlags_Borders | */
                                  ImGuiTableFlags_Resizable |
                                  ImGuiTableFlags_Hideable)) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Value");
                //ImGui::TableHeadersRow();

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Delta (ms)");
                ImGui::TableNextColumn();
                ImGui::Text("%.3f", comp->delta * 1000.0f);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Total (s)");
                ImGui::TableNextColumn();
                ImGui::Text("%.1f", comp->totalElapsed);

                ImGui::EndTable();
            }
        }
    }

    void EngineMain::ecsSystemGui(ecs::EntityHandle e)
    {
        auto comp = e.get<ecs::System>();
        if (comp) {
            if (ImGui::BeginTable("ComponentGui", 2, /*ImGuiTableFlags_Borders | */
                                  ImGuiTableFlags_Resizable |
                                  ImGuiTableFlags_Hideable)) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Value");
                //ImGui::TableHeadersRow();

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Type");
                ImGui::TableNextColumn();
                ImGui::Text("%s", comp->query ? "Query" : (comp->stream ? "Stream" : "Execute"));

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Group");
                ImGui::TableNextColumn();
                ImGui::Text("%s", e.world->description(comp->groupId).c_str());

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Thread");
                ImGui::TableNextColumn();
                ImGui::Text("%s", comp->thread ? "True" : "False");

                if (comp->stream) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Stream");
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", e.world->description(comp->stream).c_str());
                }

                if (comp->query) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("Query");
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", e.world->description(comp->query).c_str());
                }

                ImGui::EndTable();
            }
        }
    }

    void EngineMain::ecsStreamComponentGui(ecs::EntityHandle e)
    {
        auto comp = e.get<ecs::StreamComponent>();
        if (comp) {
            if (ImGui::BeginTable("ComponentGui", 2, /*ImGuiTableFlags_Borders | */
                                  ImGuiTableFlags_Resizable |
                                  ImGuiTableFlags_Hideable)) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Value");
                //ImGui::TableHeadersRow();

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Count");
                ImGui::TableNextColumn();
                ImGui::Text("%d", comp->ptr->column->count);

                ImGui::EndTable();
            }
        }
    }
}
