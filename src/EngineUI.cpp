#include "EngineMain.hpp"
#include "optick/optick.h"
#include "imgui.h"
#include "Modules/ImGui/ImGuiRender.hpp"
#include "Modules/Renderer/Renderer.hpp"

namespace RxEngine
{
    void frameStatsGui(ecs::World *, void * ptr)
    {
        auto fs = static_cast<FrameStats *>(ptr);
        if (fs) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Frame ID");
            ImGui::TableNextColumn();
            ImGui::Text("%lld", fs->frameNo);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Frame Index");
            ImGui::TableNextColumn();
            ImGui::Text("%d", fs->index);
        }
    }

    void EngineMain::ecsMainMenu(bool & show_entity_window,
                                 bool & show_systems_window,
                                 bool & show_singletons_window)
    {
        if (ImGui::BeginMenu("ECS")) {
            if (ImGui::MenuItem("Entities", nullptr, show_entity_window)) {
                show_entity_window = !show_entity_window;
                setBoolConfigValue("editor", "ecsEntityWindow", show_entity_window);
            }
            if (ImGui::MenuItem("Singletons", nullptr, show_singletons_window)) {
                show_singletons_window = !show_singletons_window;
                setBoolConfigValue("editor", "ecsSingletonsWindow", show_singletons_window);
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
        OPTICK_EVENT()
        if (table->entities.size() == 0) {
            return;
        }
#if 0
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TableNextColumn();
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4) ImColor::HSV(0.5, 0.6f, 0.6f));

        ImGui::Button(table->description().c_str());
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
#endif
        for (auto e: *table) {

            ImGui::PushID(static_cast<int>(e.id));
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%lld", e.id);
            ImGui::TableNextColumn();
            //ImGui::Text("%s", e.path().c_str());
            if (ImGui::Selectable(
                e.description().c_str(),
                selectedEntity == e
            )) {
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
            bool open = ImGui::TreeNodeEx(
                object.description().c_str(),
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
        OPTICK_EVENT()
        for (auto c: entity) {

            if (const auto cd = world->getComponentDetails(c); !cd->isRelation) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4) ImColor::HSV(0.7f, 0.6f, 0.6f));
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
                    if (ImGui::BeginTable(
                        "ComponentGui", 2, /*ImGuiTableFlags_Borders | */
                        ImGuiTableFlags_Resizable |
                        ImGuiTableFlags_Hideable
                    )) {
                        ImGui::TableSetupColumn("Name");
                        ImGui::TableSetupColumn("Value");

                        world->get<ComponentGui>(c)->
                            editor(world.get(), world->getUpdate(entity.id, c));

                        ImGui::EndTable();
                    }
                }
            } else {
                auto r = reinterpret_cast<const ecs::Relation *>(entity.getWorld()->get(
                    entity.id, c
                ));
                auto object = entity.getHandle(r->entity);

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                bool open = ImGui::TreeNodeEx(
                    object.description().c_str(),
                    ImGuiTreeNodeFlags_SpanFullWidth,
                    "%s(%s)", cd->name.c_str(),
                    object.description().c_str());
                ImGui::TableNextColumn();
                if (world->has<ComponentGui>(c)) {
                    if (ImGui::BeginTable(
                        "ComponentGui", 2, /*ImGuiTableFlags_Borders | */
                        ImGuiTableFlags_Resizable |
                        ImGuiTableFlags_Hideable
                    )) {
                        ImGui::TableSetupColumn("Name");
                        ImGui::TableSetupColumn("Value");

                        world->get<ComponentGui>(c)->
                            editor(world.get(), world->getUpdate(entity.id, c));

                        ImGui::EndTable();
                    }
                } else {
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
        OPTICK_EVENT()
        bool is_open = show_entity_window;
        static ecs::EntityHandle selectedEntity;
        static uint16_t selectedArchetype = 0;

        if (ImGui::Begin("Entities", &is_open)) {

            if (ImGui::BeginCombo(
                "Archetype",
                world->getTableForArchetype(selectedArchetype)->description().
                    c_str())) {
                for (auto it: *world) {
                    auto table = world->getTableForArchetype(it.id);
                    if (table && table->entities.size() > 0) {
                        if (ImGui::Selectable(
                            table->description().c_str(),
                            it.id == selectedArchetype
                        )) {
                            selectedArchetype = it.id;
                        }
                    }
                }
                ImGui::EndCombo();
            }
            if (ImGui::BeginTable(
                "Entities", 2,
                ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit |
                ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg,
                ImVec2(0, 400))) {
                ImGui::TableSetupColumn("ID");
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

                //world_->type()

                //for (auto it: *world) {


                //table_index++;
                //if (labels[selected_index] == table_type.str().c_str()) {
                //                    if (it.count() > 0) {
                ecsInspectorShowTableDetails(
                    selectedEntity,
                    world->getTableForArchetype(selectedArchetype));
                //                  }
                //}
                ImGui::EndTable();
            }
            if (ImGui::BeginTable(
                "FocusedEntity", 2,
                ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
                ImGuiTableFlags_Resizable | ImGuiWindowFlags_NoBackground |
                ImGuiTableFlags_NoBordersInBody
            )) {
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

                    bool open = ImGui::TreeNodeEx(
                        entity.description().c_str(),
                        ImGuiTreeNodeFlags_SpanFullWidth,
                        "%s (%s) %lld", type.c_str(),
                        entity.description().c_str(),
                        entity.id
                    );
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

    void EngineMain::ecsSingletonsWindow(bool & show_singletons_window)
    {
        OPTICK_EVENT()
        bool is_open = show_singletons_window;

        static ecs::component_id_t selectedSingleton = 0;

        if (ImGui::Begin("Singletons", &is_open)) {

            if (ImGui::BeginTable(
                "Singletons", 1,
                ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit |
                ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg,
                ImVec2(0, 400))) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

                for (auto &[k, v]: world->allSingletons()) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    if (ImGui::Selectable(
                        world->description(k).c_str(),
                        selectedSingleton == k
                    )) {
                        selectedSingleton = k;
                    }
                }
                ImGui::EndTable();
            }
            if (ImGui::BeginTable(
                "FocusedSingleton", 2,
                ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
                ImGuiTableFlags_Resizable | ImGuiWindowFlags_NoBackground |
                ImGuiTableFlags_NoBordersInBody
            )) {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
                //                ImGui::TableSetupColumn("Role", ImGuiTableColumnFlags_NoHide);
                //ImGui::TableSetupColumn("Relation", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_NoHide);
                ImGui::TableHeadersRow();

                if (selectedSingleton && world->has<ComponentGui>(selectedSingleton)) {

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    world->get<ComponentGui>(selectedSingleton)->editor(
                        world.get(), world->getSingletonUpdate(selectedSingleton));
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();

        if (is_open != show_singletons_window) {
            show_singletons_window = is_open;

            setBoolConfigValue("editor", "ecsSingletonsWindow", show_singletons_window);
        }
    }

    void EngineMain::showSystemsGui(bool & showWindow)
    {
        if (ImGui::Begin("Systems", &showWindow)) {

            auto pgs = world->getPipelineGroupSequence();

            if (ImGui::BeginTable(
                "Pipeline", 4,
                ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit |
                ImGuiTableFlags_ScrollY,// | ImGuiTableFlags_RowBg,
                ImVec2(0, 0))) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Active");
                ImGui::TableSetupColumn("Count");
                ImGui::TableSetupColumn("Time (ms)");
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

                for (auto pg: pgs) {
                    auto sg = world->get<ecs::SystemGroup>(pg);

                    ImGui::PushID(world->description(pg).c_str());
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    //ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
                    //ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, static_cast<ImVec4>(ImColor(226, 255, 138)));
                    bool open = ImGui::TreeNodeEx(world->description(pg).c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
                    ImGui::PopStyleColor(1);

                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    ImGui::Text("%lld", sg->systems.size());
                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f", sg->lastTime * 1000.f);
                    if (sg && open) {
                        for (auto e : sg->executionSequence) {
                            auto sys = world->get<ecs::System>(e);
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::Text("%s", world->description(e).c_str());
                            ImGui::TableNextColumn();
                            ImGui::Text("%s", sys->enabled ? "Active" : "Inactive");
                            ImGui::TableNextColumn();
                            ImGui::Text("%lld", sys->count);
                            ImGui::TableNextColumn();
                            ImGui::Text("%.3f", sys->executionTime * 1000.f);
                        }
                    }
                    if (open) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::PushStyleColor(ImGuiCol_Text, static_cast<ImVec4>(ImColor(138, 233, 255)));
                        ImGui::Text("Deferred Processing");
                        ImGui::PopStyleColor();

                        ImGui::TableNextColumn();
                        ImGui::TableNextColumn();
                        ImGui::Text("%lld", sg->deferredCount);
                        ImGui::TableNextColumn();
                        ImGui::Text("%.3f", sg->deferredTime * 1000.f);
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
#if 0
                for (auto x : v) {
                    if (x == 0) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("Merge");
                    }
                    else {
                        ecs_system_stats_t* s1 = ecs_map_get(
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
#endif
            }
            ImGui::EndTable();

        }
        ImGui::End();
    }

    void EngineMain::updateEntityGui()
    {
        static bool show_entity_window = getBoolConfigValue("editor", "ecsEntityWindow", false);
        static bool show_systems_window = getBoolConfigValue("editor", "ecsSystemsWindow", false);
        static bool show_singletons_window = getBoolConfigValue(
            "editor", "ecsSingletonsWindow", false
        );

        auto & io = ImGui::GetIO();
        if (io.UserData && !static_cast<IMGuiRender *>(io.UserData)->isEnabled()) {
            return;
        }

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Subsystems")) {
                ecsMainMenu(show_entity_window, show_systems_window, show_singletons_window);
            }
            ImGui::EndMainMenuBar();
        }

        if (show_entity_window) {
            bool is_open = show_entity_window;
            ecsInspectorEntityWindow(show_entity_window);
            if (is_open != show_entity_window) {
                setBoolConfigValue("editor", "ecsEntityWindow", show_entity_window);
            }
        }
#if 0
        if (show_components_window) {
            bool is_open = show_components_window;

            if (ImGui::Begin("Components", &is_open)) {}
            ImGui::End();

            if (is_open != show_components_window) {
                show_components_window = is_open;
                setBoolConfigValue("editor", "ecsComponentsWindow", show_components_window);
            }
        }
#endif
        if (show_singletons_window) {
            bool is_open = show_singletons_window;

            ecsSingletonsWindow(show_singletons_window);

            if (is_open != show_singletons_window) {
                setBoolConfigValue("editor", "ecsSingletonsWindow", show_singletons_window);
            }
        }
        if (show_systems_window) {
            bool is_open = show_systems_window;

            showSystemsGui(show_systems_window);

            if (is_open != show_systems_window) {
                setBoolConfigValue("editor", "ecsSystemsWindow", show_systems_window);
            }
        }
    }

    void ecsNameGui(ecs::World *, void * ptr)
    {
        auto name = static_cast<ecs::Name *>(ptr);
        if (name) {

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Name");
            ImGui::TableNextColumn();
            ImGui::Text("%s", name->name.c_str());
        }
    }

    void ecsComponentGui(ecs::World *, void * ptr)
    {
        auto comp = static_cast<ecs::Component *>(ptr);
        if (comp) {

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
        }
    }

    void ecsSystemGroupGui(ecs::World *, void * ptr)
    {
        auto group = static_cast<ecs::SystemGroup *>(ptr);
        if (group) {

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
            ImGui::Text("%.3f", static_cast<double>(group->delta));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Rate");
            ImGui::TableNextColumn();
            ImGui::Text("%.3f", static_cast<double>(group->rate));
        }
    }

    void ecsWindowDetailsGui(ecs::World *, void * ptr)
    {
        auto window_details = static_cast<WindowDetails *>(ptr);
        if (window_details) {

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Width");
            ImGui::TableNextColumn();
            ImGui::Text("%d", window_details->width);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Height");
            ImGui::TableNextColumn();
            ImGui::Text("%d", window_details->height);
        }
    }

    void ecsEngineTimeGui(ecs::World *, void * ptr)
    {
        auto engine_time = static_cast<EngineTime *>(ptr);
        if (engine_time) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Delta (ms)");
            ImGui::TableNextColumn();
            ImGui::Text("%.3f", engine_time->delta * 1000.0);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Total (s)");
            ImGui::TableNextColumn();
            ImGui::Text("%.1f", engine_time->totalElapsed);
        }
    }

    void ecsSystemGui(ecs::World * w, void * ptr)
    {
        auto system = static_cast<ecs::System *>(ptr);
        if (system) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Type");
            ImGui::TableNextColumn();
            ImGui::Text(
                "%s", system->query
                      ? "Query"
                      : (system->stream ? "Stream" : "Execute"));

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Group");
            ImGui::TableNextColumn();
            ImGui::Text("%s", w->description(system->groupId).c_str());

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Thread");
            ImGui::TableNextColumn();
            ImGui::Text("%s", system->thread ? "True" : "False");

            if (system->stream) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Stream");
                ImGui::TableNextColumn();
                ImGui::Text("%s", w->description(system->stream).c_str());
            }

            if (system->query) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Query");
                ImGui::TableNextColumn();
                ImGui::Text("%s", w->description(system->query).c_str());
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Count");
            ImGui::TableNextColumn();
            ImGui::Text("%lld", system->count);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Time");
            ImGui::TableNextColumn();
            ImGui::Text("%.3f ms", system->executionTime * 1000.0f);
        }
    }

    void ecsStreamComponentGui(ecs::World *, void * ptr)
    {
        auto comp = static_cast<ecs::StreamComponent *>(ptr);
        if (comp) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Count");
            ImGui::TableNextColumn();
            ImGui::Text("%d", comp->ptr->column->count);
        }
    }
}
