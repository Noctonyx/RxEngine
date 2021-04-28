//
// Created by shane on 27/12/2020.
//

#include <vector>
#include <filesystem>
#include <memory>
#include "Modules/Render.h"
#include "EngineMain.hpp"
#include "Scene.h"
#include "RxCore.h"
#include "RxECS.h"
#include "Modules/Renderer/Renderer.hpp"

#include "Modules/Module.h"
#include "Modules/ImGui/ImGuiRender.hpp"
#include "Modules/Stats/Stats.h"
#include "Modules/Prototypes/Prototypes.h"
#include "Modules/StaticMesh/StaticMesh.h"
#include "Modules/Transforms/Transforms.h"
#include "Modules/WorldObject/WorldObject.h"

namespace RxEngine
{
    class StatsModule;

    void EngineMain::setupWorld() {
        window_->setWorld(world.get());

        world->newEntity("Pipeline:PreFrame").set<ecs::SystemGroup>({1, false, 0.0f, 0.0f});
        world->newEntity("Pipeline:Early").set<ecs::SystemGroup>({2, false, 0.0f, 0.0f});
        world->newEntity("Pipeline:FixedUpdate").set<ecs::SystemGroup>({3, true, 0.0f, 0.02f});
        world->newEntity("Pipeline:Update").set<ecs::SystemGroup>({4, false, 0.0f, 0.0f});
        world->newEntity("Pipeline:UpdateUi").set<ecs::SystemGroup>({5, false, 0.0f, 0.0f});
        world->newEntity("Pipeline:PreRender").set<ecs::SystemGroup>({6, false, 0.0f, 0.0f});
        world->newEntity("Pipeline:Render").set<ecs::SystemGroup>({7, false, 0.0f, 0.0f});
        world->newEntity("Pipeline:PostRender").set<ecs::SystemGroup>({8, false, 0.0f, 0.0f});
        world->newEntity("Pipeline:PostFrame").set<ecs::SystemGroup>({9, false, 0.0f, 0.0f});

        lua = new sol::state();

        setupLuaEnvironment();
        loadLuaFile("/lua/engine");
        for (auto sf : configFiles) {
            loadLuaFile(sf);
        }

        populateStartupData();

        modules.push_back(std::make_shared<Renderer>(device_->VkDevice(), world.get(),
                                                     swapChain_->imageFormat(), this));
        modules.push_back((std::make_shared<MaterialsModule>(world.get(), this)));
        modules.push_back((std::make_shared<IMGuiRender>(world.get(), this)));
        modules.push_back((std::make_shared<TransformsModule>(world.get(), this)));
        modules.push_back((std::make_shared<StatsModule>(world.get(), this)));
        modules.push_back((std::make_shared<PrototypesModule>(world.get(), this)));
        modules.push_back((std::make_shared<StaticMeshModule>(world.get(), this)));
        modules.push_back((std::make_shared<WorldObjectModule>(world.get(), this)));

        for (auto & m: modules) {
            m->registerModule();
        }

        for (auto & m: modules) {
            m->startup();
        }

        for (auto & m: modules) {
            m->enable();
        }

        window_->onResize.AddLambda(
            [&](int w, int h)
            {
                setUint32ConfigValue("window", "width", w);
                setUint32ConfigValue("window", "height", h);
            });

        startTime = std::chrono::high_resolution_clock::now();

        world->createSystem("Engine:CheckSwapchain")
             .inGroup("Pipeline:PreFrame")
             .execute([this](ecs::World * world)
             {
                 OPTICK_EVENT("Check SwapChain")
                 if (swapChain_->swapChainOutOfDate()) {
                     replaceSwapChain();
                 }
             });

        world->createSystem("Engine:AcquireImage")
             .inGroup("Pipeline:Render")
             .label<AcquireImage>()
             .execute([this](ecs::World * world)
             {
                 OPTICK_EVENT("AcquireImage")
                 const auto current_extent = swapChain_->GetExtent();

                 auto [next_swap_image_view, next_image_available, next_image_index] =
                     swapChain_->AcquireNextImage();

                 world->getStream<MainRenderImageInput>()->add<MainRenderImageInput>(
                     {
                         next_swap_image_view, next_image_available, next_image_index,
                         current_extent, submitCompleteSemaphores_[next_image_index]
                     }
                 );
             });

        world->createSystem("Engine:PresentImage")
             .inGroup("Pipeline:Render")
             .label<PresentImage>()
             .withStream<MainRenderImageOutput>()
             .execute<MainRenderImageOutput>(
                 [this](ecs::World * world, const MainRenderImageOutput * mri)
                 {
                     OPTICK_GPU_FLIP(nullptr)
                     OPTICK_CATEGORY("Present", Optick::Category::Rendering)

                     swapChain_->PresentImage(mri->imageView, mri->finishRenderSemaphore);
                     return true;
                 });

        world->createSystem("Engine:Clean")
             .inGroup("Pipeline:PostFrame")
             .execute([](ecs::World * world)
             {
                 RxCore::JobManager::instance().clean();
                 RxCore::JobManager::threadData().freeAllResources();
             });

        world->createSystem("Engine:ECSMain")
             .inGroup("Pipeline:UpdateUi")
             .execute([this](ecs::World* world)
             {
                 OPTICK_EVENT("Engine GUI")
                 updateEntityGui();
             });

        world->set<ComponentGui>(world->getComponentId<ecs::Name>(), { .editor = ecsNameGui });
        world->set<ComponentGui>(world->getComponentId<ecs::SystemGroup>(), { .editor = ecsSystemGroupGui });
        world->set<ComponentGui>(world->getComponentId<WindowDetails>(), { .editor = ecsWindowDetailsGui });
        world->set<ComponentGui>(world->getComponentId<EngineTime>(), { .editor = ecsEngineTimeGui });
        world->set<ComponentGui>(world->getComponentId<ecs::StreamComponent>(), { .editor = ecsStreamComponentGui });
        world->set<ComponentGui>(world->getComponentId<ecs::System>(), { .editor = ecsSystemGui });
    }

    void EngineMain::startup()
    {
        loadConfig();
        uint32_t height = 1080;
        uint32_t width = 1920;

        width = getUint32ConfigValue("window", "width", 1280);
        height = getUint32ConfigValue("window", "height", 768);

        if (width <= 0) {
            width = 800;
        }
        if (height <= 0) {
            height = 600;
        }

        world = std::make_unique<ecs::World>();

        window_ = std::make_unique<Window>(width, height, "RX");

        device_ = std::make_unique<RxCore::Device>(window_->GetWindow());

        auto surface = RxCore::Device::Context()->surface;

        swapChain_ = surface->CreateSwapChain();
        swapChain_->setSwapChainOutOfDate(true);

        timer_ = std::chrono::high_resolution_clock::now();

        setupWorld();
    }

    void EngineMain::shutdown()
    {
        for (auto & m: modules) {
            m->disable();
        }

        RxCore::iVulkan()->WaitIdle();

        device_->clearQueues();
        RxCore::JobManager::instance().Shutdown();

        for (auto & m: modules) {
            m->shutdown();
        }
        for (auto & m: modules) {
            m->deregisterModule();
        }
        modules.clear();

        world.reset();
        if(lua) {
            delete lua;
        }

        destroySemaphores();
        swapChain_.reset();

        window_.reset();
        device_.reset();
    }

    void EngineMain::update()
    {
        OPTICK_FRAME("MainThread")

        const auto last_clock = timer_;
        const auto time_now = std::chrono::high_resolution_clock::now();

        const std::chrono::duration<float> delta_time = time_now - last_clock;
        totalElapsed_ = std::chrono::duration<float>(time_now - startTime).count();

        delta_ = delta_time.count();
        timer_ = time_now;

        {
            OPTICK_EVENT("Window Updates")
            window_->Update(); // Collect the window events
        }

        world->setSingleton<EngineTime>({delta_, totalElapsed_});
        {
            OPTICK_EVENT("Step World")
            world->step(delta_);
        }
    }

    void EngineMain::run()
    {
        while (!window_->ShouldClose()) {
            update();
        }
    }

    void EngineMain::replaceSwapChain()
    {
        RxCore::Device::Context()->WaitIdle();

        if (swapChain_->imageCount() != submitCompleteSemaphores_.size()) {
            destroySemaphores();
            createSemaphores(swapChain_->imageCount());
        }

        swapChain_.reset();
        RxCore::Device::Context()->surface->updateSurfaceCapabilities();
        swapChain_ = RxCore::Device::Context()->surface->CreateSwapChain();
    }

    void EngineMain::createSemaphores(const uint32_t semaphoreCount)
    {
        for (uint32_t i = 0; i < semaphoreCount; i++) {
            auto s = RxCore::Device::VkDevice().createSemaphore({});
            submitCompleteSemaphores_.push_back(s);
        }
    }

    void EngineMain::destroySemaphores()
    {
        for (const auto & semaphore: submitCompleteSemaphores_) {
            RxCore::Device::VkDevice().destroySemaphore(semaphore);
        }
        submitCompleteSemaphores_.clear();
    }

    void EngineMain::loadConfig()
    {
        const mINI::INIFile file("engine.ini");
        file.read(iniData);
    }

    void EngineMain::saveConfig()
    {
        const mINI::INIFile file("engine.ini");
        file.write(iniData, true);
    }

    std::string EngineMain::getConfigValue(const std::string & section, const std::string & entry)
    {
        return iniData[section][entry];
    }

    void EngineMain::setConfigValue(
        const std::string & section,
        const std::string & entry,
        const std::string & value)
    {
        iniData[section][entry] = value;
        saveConfig();
    }

    uint32_t EngineMain::getUint32ConfigValue(
        const std::string & section,
        const std::string & entry,
        const uint32_t defaultValue)
    {
        uint32_t v; // = defaultValue;
        try {
            v = std::stol(getConfigValue(section, entry));
        } catch (std::invalid_argument &) {
            v = defaultValue;
        }
        return v;
    }

    void EngineMain::addInitConfigFile(const std::string & config)
    {
        configFiles.push_back(config);
    }

    void EngineMain::setUint32ConfigValue(const std::string & section,
                                          const std::string & entry,
                                          const uint32_t value)
    {
        setConfigValue(section, entry, std::to_string(value));
    }

    bool EngineMain::getBoolConfigValue(const std::string & section,
                                        const std::string & entry,
                                        const bool defaultValue)
    {
        const std::string v = getConfigValue(section, entry);
        if (v.empty()) {
            return defaultValue;
        }
        return v == "1";
    }

    void EngineMain::setBoolConfigValue(const std::string & section,
                                        const std::string & entry,
                                        const bool value)
    {
        setConfigValue(section, entry, value ? "1" : "0");
    }
} // namespace RXEngine
