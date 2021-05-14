#include "Stats.h"

#include "World.h"
#include "imgui.h"
#include "EngineMain.hpp"

namespace RxEngine
{
    void StatsModule::startup()
    {
        systemSet_ = world_->newEntity().set<ecs::SystemSet>({false}).id;

        world_->createSystem("Stats:Ui")
              .withSet(systemSet_)
              .inGroup("Pipeline:UpdateUi")           
              .execute([this](ecs::World * w)
                  {
                      delta_ = w->deltaTime();
                      fps_ = fps_ * 0.99f + 0.01f * (1 / delta_);
                      presentStatsUi();
                  }
              );

        world_->createSystem("Stats:Mem")
              .withSet(systemSet_)
              .inGroup("Pipeline:Update")              
              .execute([this](ecs::World *)
              {
                  RxCore::iVulkan()->getMemBudget(heaps_);
              });
    }

    void StatsModule::enable()
    {
        if (systemSet_) {
            world_->getUpdate<ecs::SystemSet>(systemSet_)->enabled = true;
        }
    }

    void StatsModule::disable()
    {
        if (systemSet_) {
            world_->getUpdate<ecs::SystemSet>(systemSet_)->enabled = false;
        }
    }

    void StatsModule::shutdown()
    {
        world_->deleteSystem(world_->lookup("Stats:Mem").id);
        world_->deleteSystem(world_->lookup("Stats:Ui").id);
    }

    void StatsModule::presentStatsUi()
    {
        OPTICK_EVENT()

        bool p_open = true;
        const float DISTANCE = 10.0f;
        auto & io = ImGui::GetIO();
        ImVec2 window_pos = ImVec2(io.DisplaySize.x - 2, 2 * DISTANCE);

        fpsHistory_.push_back(delta_ * 1000.f);
        while (fpsHistory_.size() > 250) {
            fpsHistory_.pop_front();
        }

        std::vector<float> fpss;
        std::vector<float> gpus;
        fpss.resize(fpsHistory_.size());
        gpus.resize(gpuHistory_.size());

        std::ranges::transform(
            fpsHistory_.begin(), fpsHistory_.end(), fpss.begin(), [](float f)
            {
                return f;
            });
        std::ranges::transform(
            gpuHistory_.begin(), gpuHistory_.end(), gpus.begin(), [](float f)
            {
                return f;
            });

        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
        if (ImGui::Begin(
            "Overlay",
            &p_open,
            (ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                ImGuiWindowFlags_NoNav))) {
            ImGui::Text("FPS %6.1f FPS", fps_);
            ImGui::PlotLines(
                "16ms",
                fpss.data(),
                static_cast<uint32_t>(fpss.size()),
                0,
                "Frame Time",
                0.f,
                20.f,
                ImVec2(0, 50));
            ImGui::PlotLines(
                "16ms",
                gpus.data(),
                static_cast<uint32_t>(gpus.size()),
                0,
                "GPU Time",
                0.f,
                16.f,
                ImVec2(0, 50));
            ImGui::Text("Frame Time %6.2f ms", delta_ * 1000.f);
            ImGui::Text("Render CPU Time: %5.2f ms", 0.f);
            ImGui::Text("Render GPU Time: %5.2f ms", 0.f);
            for (const auto & heap: heaps_) {
                std::ostringstream stringStream;
                stringStream << (heap.usage / 1024 / 1024) << "MB/" << (heap.budget / 1204 / 1204)
                    << "MB";
                std::string copyOfStr = stringStream.str();
                ImGui::ProgressBar(
                    static_cast<float>(heap.usage) / static_cast<float>(heap.budget),
                    ImVec2(-1, 0),
                    stringStream.str().c_str());
            }
        }
        ImGui::End();
    }
}
