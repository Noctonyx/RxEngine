#include "Stats.hpp"
#include "imgui.h"

namespace RxEngine
{
    void Stats::UpdateGui()
    {
        OPTICK_EVENT()
        bool p_open = true;

        auto & io = ImGui::GetIO();
        const float DISTANCE = 10.0f;
        ImVec2 window_pos = ImVec2(io.DisplaySize.x - 2, 2 * DISTANCE);

        fpsHistory_.push_back(delta_ * 1000.f);
        while (fpsHistory_.size() > 250) {
            fpsHistory_.pop_front();
        }
        gpuHistory_.push_back(static_cast<float>(renderer_->gpuTime));
        while (gpuHistory_.size() > 250) {
            gpuHistory_.pop_front();
        }

        std::vector<RxCore::MemHeapStatus> heaps;
        RxCore::iVulkan()->getMemBudget(heaps);

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
//        RXCore::iVulkan()->physicalDevice->

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
            ImGui::Text("Render CPU Time: %5.2f ms", renderer_->cpuTime);
            ImGui::Text("Render GPU Time: %5.2f ms", renderer_->gpuTime);
            for (const auto & heap : heaps) {
                std::ostringstream stringStream;
                stringStream << (heap.usage / 1024 / 1024) << "MB/" << (heap.budget / 1204 / 1204) << "MB";
                std::string copyOfStr = stringStream.str();
                ImGui::ProgressBar(
                    static_cast<float>( heap.usage) / static_cast<float>(heap.budget),
                    ImVec2(-1, 0),
                    stringStream.str().c_str());
            }
        }
        ImGui::End();
    }

    void Stats::Update(const float delta)
    {
        delta_ = delta;
        fps_ = fps_ * 0.99f + 0.01f * (1 / delta_);
    }

    void Stats::rendererInit(Renderer * renderer) { renderer_ = renderer; }

    std::vector<RenderEntity> Stats::getRenderEntities()
    {
        return {};
    }

} // namespace RXCore
