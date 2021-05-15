#include "Lighting.h"
#include "EngineMain.hpp"
//#include "RxCore.h"

constexpr int lighting_buffer_count = 5;

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

                  ds->ds->updateDescriptor(1, vk::DescriptorType::eUniformBufferDynamic,
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
    }

    void LightingModule::shutdown()
    {
        world_->deleteSystem(world_->lookup("Lighting:updateDescriptor"));
        world_->deleteSystem(world_->lookup("Lighting:setDescriptor"));
        world_->deleteSystem(world_->lookup("Lighting:NextFrame"));

        world_->removeSingleton<Lighting>();
    }
}
