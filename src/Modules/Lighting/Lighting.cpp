#include <Modules/Renderer/Renderer.hpp>
#include "Lighting.h"
#include "EngineMain.hpp"
#include "Modules/RTSCamera/RTSCamera.h"
#include "Modules/SceneCamera/SceneCamera.h"
#include "Vulkan/Buffer.hpp"
//#include "RxCore.h"

constexpr int lighting_buffer_count = 5;

using namespace DirectX;

namespace RxEngine
{
    void LightingModule::startup()
    {
        world_->addSingleton<Lighting>();
        auto lighting = world_->getSingletonUpdate<Lighting>();

        lighting->bufferAlignment = engine_->getUniformBufferAlignment(sizeof(LightingShaderData));
        lighting->lightingBuffer = engine_->createUniformBuffer(
            lighting_buffer_count * lighting->bufferAlignment);

        lighting->lightingBuffer->map();
        lighting->ix = 0;

        lighting->shaderData.specularPower = 56;
        lighting->shaderData.specularStrength = 0.21f;
        lighting->shaderData.ambientStrength = 0.64f;
        lighting->shaderData.diffAmount = 0.3f;
        lighting->shaderData.light_direction = {0.407f, -.707f, 0.8f};


        world_->createSystem("Lighting:NextFrame")
              .inGroup("Pipeline:PreRender")
              .withWrite<Lighting>()
              .execute([](ecs::World * world)
                  {
                      const auto sc = world->getSingletonUpdate<Lighting>();

                      sc->ix = (sc->ix + 1) % lighting_buffer_count;
                      sc->lightingBuffer->update(&sc->shaderData,
                                                 sc->ix * sc->bufferAlignment,
                                                 sizeof(LightingShaderData));
                  }
              );

        world_->createSystem("Lighting:setDescriptor")
              .inGroup("Pipeline:PreFrame")
              .withQuery<DescriptorSet>()
              .without<LightingDescriptor>()
              .withRead<Lighting>()
              .each<DescriptorSet>([](ecs::EntityHandle e, DescriptorSet * ds)
              {
                  auto sc = e.world->getSingleton<Lighting>();

                  ds->ds->updateDescriptor(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                                           sc->lightingBuffer, sizeof(LightingShaderData),
                                           static_cast<uint32_t>(sc->ix * sc->bufferAlignment));

                  e.addDeferred<LightingDescriptor>();
              });


        world_->createSystem("Lighting:updateDescriptor")
              .inGroup("Pipeline:PreRender")
              .withQuery<DescriptorSet, LightingDescriptor>()
              .withRead<Lighting>()
              //.withRead<SceneCameraShaderData>()
              .each<DescriptorSet>([](ecs::EntityHandle e, const DescriptorSet * ds)
              {
                  auto sc = e.world->getSingleton<Lighting>();
                  ds->ds->setDescriptorOffset(1, sc->getDescriptorOffset());
              });

        world_->createSystem("Lighting:CreateCascadeData")
              .inGroup("Pipeline:PreRender")
              .withRead<SceneCamera>()
              .withRead<RTSCamera>()
              .withRead<CameraProjection>()
              .withWrite<ShadowCascadeData>()
              .execute<>([this](ecs::World * w)
              {
                  createShadowCascadeData();
              });
    }

    void LightingModule::shutdown()
    {
        world_->deleteSystem(world_->lookup("Lighting:updateDescriptor"));
        world_->deleteSystem(world_->lookup("Lighting:setDescriptor"));
        world_->deleteSystem(world_->lookup("Lighting:NextFrame"));

        world_->removeSingleton<Lighting>();
    }

    void LightingModule::calculateCascades(const uint32_t numberCascades,
                                           const RTSCamera * camera,
                                           const CameraProjection * proj,
                                           ShadowCascadeData & scd,
                                           XMFLOAT3 lightDirection)
    {
        float clipRange = std::max(proj->farZ, 100.0f) - proj->nearZ;

        float minZ = proj->nearZ;
        float maxZ = proj->nearZ + clipRange;

        float range = maxZ - minZ;
        float ratio = maxZ / minZ;


        float cascadeSplitLambda = 0.965f;

        scd.cascadeSplits.resize(numberCascades);
        scd.cascades.resize(numberCascades);

        for (uint32_t i = 0; i < numberCascades; i++) {
            float p = static_cast<float>(i + 1) / static_cast<float>(numberCascades);
            float log = minZ * std::pow(ratio, p);
            float uniform = minZ + range * p;
            float d = cascadeSplitLambda * (log - uniform) + uniform;
            scd.cascadeSplits[i] = (d - proj->nearZ) / clipRange;
        }


        float lastSplitDist = 0.0;

        for (uint32_t i = 0; i < numberCascades; i++) {
            float splitDist = scd.cascadeSplits[i];

            XMVECTOR frustum_corners[8];

            frustum_corners[0] = XMVectorSet(-1.f, 1.f, -1.f, 0);
            frustum_corners[1] = XMVectorSet(1.f, 1.f, -1.f, 0);
            frustum_corners[2] = XMVectorSet(1.f, -1.f, -1.f, 0);
            frustum_corners[3] = XMVectorSet(-1.f, -1.f, -1.f, 0);
            frustum_corners[4] = XMVectorSet(-1.f, 1.f, 1.f, 0);
            frustum_corners[5] = XMVectorSet(1.f, 1.f, 1.f, 0);
            frustum_corners[6] = XMVectorSet(1.f, -1.f, 1.f, 0);
            frustum_corners[7] = XMVectorSet(-1.f, -1.f, 1.f, 0);

            XMMATRIX inCam = XMLoadFloat4x4(&camera->iViewProj);

            for (auto & frustum_corner: frustum_corners) {
                frustum_corner = XMVector3TransformCoord(frustum_corner, inCam);
            }

            for (uint32_t j = 0; j < 4; j++) {
                const auto dist = XMVectorSubtract(frustum_corners[j + 4],
                                                   frustum_corners[j]);

                frustum_corners[j + 4] = XMVectorAdd(
                    frustum_corners[j],
                    XMVectorScale(dist, splitDist)
                );
                frustum_corners[j] = XMVectorAdd(
                    frustum_corners[j],
                    XMVectorScale(dist, lastSplitDist)
                );
            }

            XMVECTOR frustumCenter = XMVectorZero();

            for (auto & frustum_corner: frustum_corners) {
                frustumCenter = XMVectorAdd(frustumCenter, frustum_corner);
            }
            frustumCenter = XMVectorScale(frustumCenter, 1.f / 8.0f);

            XMVECTOR radius = XMVectorZero();

            for (auto & frustum_corner: frustum_corners) {
                auto d = XMVector3Length(XMVectorSubtract(frustum_corner, frustumCenter));
                radius = XMVectorMax(d, radius);
            }

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
                                      {extent, extent, extent * 2.0f},
                                      {0.f, 0.f, 0.f, 1.f});
            BoundingOrientedBox bobox2;
            bobox.Transform(bobox2, lightView);
            XMStoreFloat3(&bobox2.Center, frustumCenter);
            auto lightProj = XMMatrixOrthographicRH(extent * 2.0f, extent * 2.0f, 0.f,
                                                    extent * 2.0f);

            // Store split distance and matrix in cascade
            scd.cascades[i].splitDepth = (proj->nearZ + splitDist * clipRange) * -1.0f;
            XMStoreFloat4x4(&scd.cascades[i].viewMatrix, lightView);
            scd.cascades[i].boBox = bobox2;
            XMStoreFloat4x4(&scd.cascades[i].viewProjMatrix, lightView * lightProj);

            lastSplitDist = scd.cascadeSplits[i];
        }
    }

    void LightingModule::createShadowCascadeData()
    {
        const int numberCascades = 4;

        auto sc = world_->getSingleton<SceneCamera>();
        auto camera = world_->get<RTSCamera>(sc->camera);
        auto proj = world_->get<CameraProjection>(sc->camera);

        ShadowCascadeData scd;
        XMFLOAT3 lightDirection = {0.407f, -.707f, 0.8f};

        calculateCascades(numberCascades, camera, proj, scd, lightDirection);

        world_->setSingleton<ShadowCascadeData>(scd);
    }
}
