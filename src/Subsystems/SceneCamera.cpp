#include <memory>
#include "SceneCamera.h"
#include "Scene.h"
#include "imgui.h"

namespace RxEngine
{
#if 0
    void SceneCamera::UpdateGui()
    {
        OPTICK_EVENT()

        //glm::mat4 proj;
        //glm::mat4 view;
        //camera_->frustum_.getProjectedBounds(glm::vec3(-1, -.00f, 0), glm::vec3(0, 0, 1), proj, view);

        const float DISTANCE = 5.0f;

        auto & io = ImGui::GetIO();
        ImVec2 window_pos = ImVec2(
            io.DisplaySize.x - 2 * DISTANCE,
            io.DisplaySize.y - 2 * DISTANCE);

        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(1.0f, 1.0f));
        ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
        if (ImGui::Begin(
            "Camera Overlay",
            nullptr,
            (ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
             ImGuiWindowFlags_NoNav))) {

            DirectX::XMFLOAT3 fwd;
            DirectX::XMFLOAT3 up;
            DirectX::XMFLOAT3 right;
            DirectX::XMStoreFloat3(&fwd, camera_->getForward());
            DirectX::XMStoreFloat3(&up, camera_->getUp());
            DirectX::XMStoreFloat3(&right, camera_->getRight());
//            auto fwd = camera_->getForward();
  //          auto up = camera_->getUp();
    //        auto right = camera_->getRight();
            ImGui::Text(
                "Base Position: %.3f, %.3f, %.3f", camera_->basePosition.x,
                camera_->basePosition.y,
                camera_->basePosition.z);
            ImGui::Text(
                "Cam Position: %.3f, %.3f, %.3f", camera_->cameraShaderData.viewPos.x,
                camera_->cameraShaderData.viewPos.y,
                camera_->cameraShaderData.viewPos.z);
            ImGui::Text(
                "X/Y/Z Rotation: %.3f, %.3f, %.3f",
                camera_->rotation.x,
                camera_->rotation.y,
                camera_->rotation.z);
            ImGui::Text("Arm Length: %.3f", camera_->dolly.z);
            ImGui::Text("Forward: %.3f, %.3f, %.3f", fwd.x, fwd.y, fwd.z);
            ImGui::Text("Up: %.3f, %.3f, %.3f", up.x, up.y, up.z);
            ImGui::Text("Right: %.3f, %.3f, %.3f", right.x, right.y, right.z);
            //ImGui::Text("ShadowBox Min: %.3f, %.3f, %.3f", b.getMin().x, b.getMin().y, b.getMin().z);
            //ImGui::Text("ShadowBox Max: %.3f, %.3f, %.3f", b.getMax().x, b.getMax().y, b.getMax().z);
        }
        ImGui::End();
    }

    void SceneCamera::addedToScene(
        Scene * scene,
        std::vector<std::shared_ptr<Subsystem>> & subsystems)
    {
        Subsystem::addedToScene(scene, subsystems);

        //camera_->setRotation({ DirectX::XMConvertToRadians(-30.0f), 0.0f, 0.0f});
       // camera_->setPerspective(60.0f, scene->getAspectRatio(), 0.1f, 256.0f);
        //camera_->setTranslation({0.0f, .0f, 0.0f});
    }

    SceneCamera::SceneCamera()
    {
        camera_ = std::make_shared<Camera>();
#if 0
        bufferPooler_.setAllocator(
            [&]
            {
                auto b = RXCore::VulkanContext::Context()->createBuffer(
                    vk::BufferUsageFlagBits::eUniformBuffer,
                    VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(RXCore::CameraShaderData)
                );
                b->getMemory()->map();
                return b;
            });
#endif
#if 0
        bufferAlignment = RXCore::iVulkan()->getUniformBufferAlignment(sizeof(CameraShaderData));

        cameraBuffer2_ = RXCore::iVulkan()->createBuffer(
            vk::BufferUsageFlagBits::eUniformBuffer,
            VMA_MEMORY_USAGE_CPU_TO_GPU, 10 * bufferAlignment);
        cameraBuffer2_->getMemory()->map();
#endif
    }
#if 0
    void SceneCamera::setup(const std::shared_ptr<Camera> & camera)
    {
        cameraBuffer2_->getMemory()->update(
            &(camera_->cameraShaderData),
            ix * bufferAlignment,
            sizeof(CameraShaderData));
        //cameraBuffer_ = bufferPooler_.get();
        //cameraBuffer_->getMemory()->update(&(camera_->cameraShaderData), sizeof(RXCore::CameraShaderData));
    }

    void SceneCamera::teardown()
    {
        //cameraBuffer_.reset();
        ix++;
        ix = ix % 10;
    }

    void SceneCamera::updateDescriptor(const std::shared_ptr<RXCore::DescriptorSet> & set, uint32_t binding) const
    {
        set->updateDescriptor(
            binding, vk::DescriptorType::eUniformBufferDynamic, cameraBuffer2_,
            sizeof(CameraShaderData), static_cast<uint32_t>( ix * bufferAlignment));
        //offsets.push_back(static_cast<uint32_t>( ix * bufferAlignment));
    }
#endif
#endif
}
