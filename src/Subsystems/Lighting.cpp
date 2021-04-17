#include <memory>
#include "Lighting.h"
#include "imgui.h"
#include "Subsystem.h"
#include "Geometry/Camera.hpp"
#include "DirectXMath.h"

using namespace DirectX;

namespace RxEngine
{
    void Lighting::UpdateGui()
    {
        OPTICK_EVENT()

        static auto show = false;
        //static bool show_lighting_window = getBoolConfigValue("editor", "entityWindow", false);


        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Subsystems")) {
                if (ImGui::MenuItem("Lighting", nullptr, show)) {
                    show = !show;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (show) {
            if (ImGui::Begin("Lighting", &show)) {

                ImGui::DragFloat("Ambient Strength", &(lighting_.ambientStrength), 0.01f, 0.0f,
                                 1.0f);
                ImGui::DragFloat("Diffuse Strength", &(lighting_.diffAmount), 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("Specular Power", &(lighting_.specularPower), 1.00f, 1.0f, 512.0f);
                ImGui::DragFloat("Specular Strength", &(lighting_.specularStrength), .01f, 0.0f,
                                 1.0f);
                ImGui::Text(
                    "Light Dir: %.3f %.3f %.3f",
                    lighting_.light_direction.x,
                    lighting_.light_direction.y,
                    lighting_.light_direction.z
                );
                if (ImGui::DragFloat3(
                    "Light Dir",
                    reinterpret_cast<float *>(&(mainLightDirection)),
                    1.0f,
                    -180.0f,
                    180.0f)) {
                    //lighting_.light_direction = glm::radians(mainLightDirection);
#if 0
                    DirectX
                    dir = glm::quat(
                        glm::vec3(
                            glm::radians(mainLightDirection.x + 0.0f),
                            glm::radians(mainLightDirection.y),
                            glm::radians(mainLightDirection.z)));
#endif
                }
               // DirectX::XMFLOAT3 pos;


                ImGui::Text(
                    "Cascade 1 position: %.1f %.1f %.1f",
                    cascades_[0].boBox.Center.x,
                    cascades_[0].boBox.Center.y,
                    cascades_[0].boBox.Center.z
                );
                ImGui::Text(
                    "Cascade 1 size: %.1f %.1f %.1f",
                    cascades_[0].boBox.Extents.x,
                    cascades_[0].boBox.Extents.y,
                    cascades_[0].boBox.Extents.z
                );
                ImGui::Text(
                    "Cascade 2 position: %.1f %.1f %.1f",
                    cascades_[1].boBox.Center.x,
                    cascades_[1].boBox.Center.y,
                    cascades_[1].boBox.Center.z
                );
                ImGui::Text(
                    "Cascade 2 size: %.1f %.1f %.1f",
                    cascades_[1].boBox.Extents.x,
                    cascades_[1].boBox.Extents.y,
                    cascades_[1].boBox.Extents.z
                );
                ImGui::Text(
                    "Cascade 3 position: %.1f %.1f %.1f",
                    cascades_[2].boBox.Center.x,
                    cascades_[2].boBox.Center.y,
                    cascades_[2].boBox.Center.z
                );
                ImGui::Text(
                    "Cascade 3 size: %.1f %.1f %.1f",
                    cascades_[2].boBox.Extents.x,
                    cascades_[2].boBox.Extents.y,
                    cascades_[2].boBox.Extents.z
                );
                ImGui::Text(
                    "Cascade 4 position: %.1f %.1f %.1f",
                    cascades_[3].boBox.Center.x,
                    cascades_[3].boBox.Center.y,
                    cascades_[3].boBox.Center.z
                );
                ImGui::Text(
                    "Cascade 4 size: %.1f %.1f %.1f",
                    cascades_[3].boBox.Extents.x,
                    cascades_[3].boBox.Extents.y,
                    cascades_[3].boBox.Extents.z
                );

                //                ImGui::gizmo3D("##gizmo1", rotation /*, size,  mode */);
#if 0
                if (ImGui::gizmo3D(
                    "##Dir1",
                    dir,
                    100
                    /*, size,  mode */)) {
                    //lighting_.light_direction =  glm::degrees( glm::eulerAngles(mainLightDirection));
                    //lighting_.light_direction = glm::normalize(lighting_.light_direction);
                }
#endif
                ImGui::End();
            }
        }
    }

    void Lighting::rendererInit(Renderer * renderer)
    {
#if 0
        if (!renderer_) {
            renderer_ = renderer;
            renderer->setLightingManager(shared_from_this());
        }
#endif
        lighting_.specularPower = 56;
        lighting_.specularStrength = 0.21f;
        lighting_.ambientStrength = 0.64f;
        lighting_.diffAmount = 0.3f;
        lighting_.light_direction = mainLightDirection;
    }

    void Lighting::prepareCamera(const std::shared_ptr<Camera> & camera)
    {
        OPTICK_EVENT()
        //std::vector<ShadowCascade> cascades;

        lighting_.cascadeCount = NUM_CASCADES;

        createShadowCascadeViews(camera, NUM_CASCADES, cascades_, mainLightDirection);
        //camera->createShadowCascadeViews(NUM_CASCADES, cascades, mainLightDirection);
        for (uint32_t i = 0; i < NUM_CASCADES; i++) {
            lighting_.cascades[i].viewProjMatrix = cascades_[i].viewProjMatrix;
            lighting_.cascades[i].splitDepth = cascades_[i].splitDepth;
        }
    }

    void Lighting::getCascadeData(std::vector<ShadowCascade> & cascades)
    {
        cascades.resize(lighting_.cascadeCount);
        for (uint32_t i = 0; i < cascades_.size(); i++) {
            cascades[i] = cascades_[i];
        }
    }

    void Lighting::setShadowMap(std::shared_ptr<RxCore::ImageView> shadowMap)
    {
        shadowMap_ = std::move(shadowMap);
    }

    void Lighting::updateShadowMapDescriptor(const std::shared_ptr<RxCore::DescriptorSet> & set,
                                             uint32_t binding)
    {
        if (!shadowMap_) {
            return;
        }

        if (!shadowSampler_) {

            vk::SamplerCreateInfo sci;
            sci.setMagFilter(vk::Filter::eLinear)
               .setMinFilter(vk::Filter::eLinear)
               .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
               .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
               .setMaxLod(1.0f)
               .setBorderColor(vk::BorderColor::eFloatOpaqueWhite);

            shadowSampler_ = RxCore::iVulkan()->createSampler(sci);
        }
        set->updateDescriptor(
            binding,
            vk::DescriptorType::eCombinedImageSampler,
            shadowMap_,
            vk::ImageLayout::eDepthStencilReadOnlyOptimal,
            shadowSampler_);
    }

    void Lighting::setup(const std::shared_ptr<Camera> & camera)
    {
        ix++;
        ix = ix % 10;

        prepareCamera(camera);

        lighting_.specularPower = 56;
        lighting_.specularStrength = 0.21f;
        lighting_.ambientStrength = 0.64f;
        lighting_.diffAmount = 0.3f;
        lighting_.light_direction = mainLightDirection;

        lightingBuffer2_->getMemory()->update(
            &(lighting_),
            ix * bufferAlignment,
            sizeof(lighting_));
    }

    void Lighting::teardown() { }

    void Lighting::updateDescriptor(const std::shared_ptr<RxCore::DescriptorSet> & set,
                                    uint32_t binding) const
    {
        set->updateDescriptor(
            binding, vk::DescriptorType::eUniformBufferDynamic, lightingBuffer2_,
            sizeof(lighting_), static_cast<uint32_t>(ix * bufferAlignment));
    }

    void Lighting::createShadowCascadeViews(
        const std::shared_ptr<Camera> & camera,
        const uint32_t numCascades,
        std::vector<ShadowCascade> & cascades,
        const DirectX::XMFLOAT3 lightDirection) const
    {
        assert(numCascades <= MAX_SHADOW_CASCADES);

        float clipRange = std::max(camera->zFar_, 100.0f) - camera->zNear_;

        float minZ = camera->zNear_;
        float maxZ = camera->zNear_ + clipRange;

        float range = maxZ - minZ;
        float ratio = maxZ / minZ;

        std::vector<float> cascadeSplits;
        float cascadeSplitLambda = 0.965f;
        cascadeSplits.resize(numCascades);

        cascades.resize(numCascades);

        for (uint32_t i = 0; i < numCascades; i++) {
            float p = static_cast<float>(i + 1) / static_cast<float>(numCascades);
            float log = minZ * std::pow(ratio, p);
            float uniform = minZ + range * p;
            float d = cascadeSplitLambda * (log - uniform) + uniform;
            cascadeSplits[i] = (d - camera->zNear_) / clipRange;
        }

        //std::array<DirectX::XMFLOAT3, 8> corners;

        // DirectX::BoundingFrustum f1(DirectX::XMLoadFloat4x4(&camera->cameraShaderData.projection), true);
        //DirectX::BoundingFrustum f2;

        //f1.Transform(f2, DirectX::XMLoadFloat4x4(&camera->matrices.iview));
        //f2.GetCorners(corners.data());

        float lastSplitDist = 0.0;
        for (uint32_t i = 0; i < numCascades; i++) {
            float splitDist = cascadeSplits[i];

            DirectX::XMVECTOR frustumCorners[8];

            frustumCorners[0] = DirectX::XMVectorSet(-1.f, 1.f, -1.f, 0);
            frustumCorners[1] = DirectX::XMVectorSet(1.f, 1.f, -1.f, 0);
            frustumCorners[2] = DirectX::XMVectorSet(1.f, -1.f, -1.f, 0);
            frustumCorners[3] = DirectX::XMVectorSet(-1.f, -1.f, -1.f, 0);
            frustumCorners[4] = DirectX::XMVectorSet(-1.f, 1.f, 1.f, 0);
            frustumCorners[5] = DirectX::XMVectorSet(1.f, 1.f, 1.f, 0);
            frustumCorners[6] = DirectX::XMVectorSet(1.f, -1.f, 1.f, 0);
            frustumCorners[7] = DirectX::XMVectorSet(-1.f, -1.f, 1.f, 0);

            DirectX::XMMATRIX inCam = DirectX::XMLoadFloat4x4(&camera->iProjView);

            for (uint32_t j = 0; j < 8; j++) {
                frustumCorners[j] = DirectX::XMVector3TransformCoord(frustumCorners[j], inCam);
            }

            for (uint32_t j = 0; j < 4; j++) {
                auto dist = DirectX::XMVectorSubtract(frustumCorners[j + 4], frustumCorners[j]);
                frustumCorners[j + 4] = DirectX::XMVectorAdd(
                    frustumCorners[j],
                    DirectX::XMVectorScale(dist, splitDist));
                frustumCorners[j] = DirectX::XMVectorAdd(
                    frustumCorners[j],
                    DirectX::XMVectorScale(dist, lastSplitDist));

                //                glm::vec3 dist = frustumCorners[j + 4] - frustumCorners[j];
                //              frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
                //            frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
            }
#if 0
            // Project frustum corners into world space
            glm::mat4 invCam = glm::inverse(camera->cameraShaderData.projection * camera->cameraShaderData.view);
            for (uint32_t j = 0; j < 8; j++) {
                glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[j], 1.0f);
                frustumCorners[j] = invCorner / invCorner.w;
            }

            for (uint32_t j = 0; j < 4; j++) {
                glm::vec3 dist = frustumCorners[j + 4] - frustumCorners[j];
                frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
                frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
            }
#endif
            DirectX::XMVECTOR frustumCenter = DirectX::XMVectorZero();
            for (uint32_t j = 0; j < 8; j++) {
                frustumCenter = DirectX::XMVectorAdd(frustumCenter, frustumCorners[j]);
                //                frustumCenter += frustumCorners[j];
            }
            frustumCenter = DirectX::XMVectorScale(frustumCenter, 1.f / 8.0f);

            // Get frustum center
            //glm::vec3 frustumCenter = glm::vec3(0.0f);
            //            for (uint32_t j = 0; j < 8; j++) {
            //              frustumCenter += frustumCorners[j];
            //        }
#if 0
            frustumCenter /= 8.0f;
            frustumCenter.x = std::ceil(frustumCenter.x * 1024.0f) / 1024.0f;
            frustumCenter.z = std::ceil(frustumCenter.z * 1024.0f) / 1024.0f;
#endif
            DirectX::XMVECTOR radius = DirectX::XMVectorZero();

            for (uint32_t j = 0; j < 8; j++) {
                auto d = DirectX::XMVector3Length(
                    DirectX::XMVectorSubtract(frustumCorners[j], frustumCenter));
                radius = DirectX::XMVectorMax(d, radius);
            }
            //radius = DirectX::XMVectorScale(radius, 16.0f);
            //radius = DirectX::XMVectorCeiling(radius);
            //radius = DirectX::XMVectorScale(radius, 1.f / 16.f);
#if 0
            float radius = 0.0f;
            for (uint32_t j = 0; j < 8; j++) {
                float distance = glm::length(frustumCorners[j] - frustumCenter);
                radius = glm::max(radius, distance);
            }
            radius = std::ceil(radius * 16.0f) / 16.0f;
#endif
            float extent = XMVectorGetX(radius);
            auto lightDir = XMLoadFloat3(&lightDirection);
            lightDir = XMVector3Normalize(lightDir);
            
            auto viewPos = frustumCenter;
            //auto lookAt = frustumCenter;
            viewPos = XMVectorSubtract(frustumCenter, XMVectorScale(lightDir, extent * 1.0f));
            auto lightView = XMMatrixLookAtRH(viewPos, frustumCenter,
                                              XMVectorSet(0.707f, 0.707f, 0, 0));
            //XMQuaternionInverse()
            BoundingOrientedBox bobox({0.f, 0.f, 0.f},
                                      {extent , extent , extent * 2.0f},
                                      {0.f, 0.f, 0.f, 1.f});
            BoundingOrientedBox bobox2;
            bobox.Transform(bobox2, lightView); // XMMatrixInverse(nullptr, lightView));
            XMStoreFloat3(&bobox2.Center, frustumCenter);
            auto lightProj = XMMatrixOrthographicRH(extent * 2.0f, extent * 2.0f, 0.f,
                                                    extent * 2.0f);

#if 0
            glm::vec3 maxExtents = glm::vec3(radius);
            glm::vec3 minExtents = -maxExtents;

            glm::vec3 lightDir = normalize(lightDirection);
            glm::mat4 lightViewMatrix = glm::lookAt(
                frustumCenter - lightDir * -minExtents.z,
                frustumCenter,
                glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 lightOrthoMatrix = glm::ortho(
                minExtents.x,
                maxExtents.x,
                minExtents.y,
                maxExtents.y,
                0.0f,
                maxExtents.z - minExtents.z);
#endif
            // Store split distance and matrix in cascade
            cascades[i].splitDepth = (camera->zNear_ + splitDist * clipRange) * -1.0f;
            XMStoreFloat4x4(&cascades[i].viewMatrix, lightView);
            //            XMStoreFloat4x4(&cascades[i].projMatrix, lightProj);
            cascades[i].boBox = bobox2;
            XMStoreFloat4x4(&cascades[i].viewProjMatrix, lightView * lightProj);
            //            cascades[i].viewMatrix = lightViewMatrix;
            //          cascades[i].viewProjMatrix = lightOrthoMatrix * lightViewMatrix;

            lastSplitDist = cascadeSplits[i];
        }
    }

    std::vector<RenderEntity> Lighting::getRenderEntities()
    {
        return {};
    }
}
