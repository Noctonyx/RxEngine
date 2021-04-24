//
// Created by shane on 27/12/2020.
//

#include <vector>
#include <filesystem>
#include <memory>
#include "Modules/Mesh.h"
#include "Modules/Render.h"
#include "EngineMain.hpp"
#include "Scene.h"
#include "Input/Mouse.hpp"
#include "Vulkan/Surface.hpp"
#include "Rendering/Renderer.hpp"
#include "Subsystems/SceneCamera.h"
#include <RmlUi/Core.h>
#include <RmlUi/Lua.h>
#include <imgui.h>

#include "Modules/Module.h"
#include "Modules/ImGui/ImGuiRender.hpp"

//#include <sol/sol.hpp>


namespace RxEngine
{
    void EngineMain::startup()
    {
        loadConfig();
        uint32_t height = 1080;
        uint32_t width = 1920;

        width = getUint32ConfigValue("window", "width", 800);
        height = getUint32ConfigValue("window", "height", 600);
        /*
        try {
            width = std::stol(getConfigValue("window", "width"));
            height = std::stol(getConfigValue("window", "height"));
        } catch (std::invalid_argument) {
            width = 800;
            height = 600;
        }
         */
        if (width <= 0) {
            width = 800;
        }
        if (height <= 0) {
            height = 600;
        }

        world = std::make_unique<ecs::World>();

        window_ = std::make_unique<Window>(width, height, "RX", world.get());

        //world->setSingleton<MainWindow::WindowDetails>({ window_.get(), width, height });

        device_ = std::make_unique<RxCore::Device>(window_->GetWindow());
        renderer_ = std::make_unique<Renderer>(device_->VkDevice(), world.get());

        auto surface = RxCore::Device::Context()->surface;

        swapChain_ = surface->CreateSwapChain();
        swapChain_->setSwapChainOutOfDate(true);

        //window_->mouse = std::make_shared<Mouse>(window_.get(), world.get());
        //window_->keyboard = std::make_shared<Keyboard>(window_.get(), world.get());

        renderer_->startup(swapChain_->imageFormat());

        timer_ = std::chrono::high_resolution_clock::now();

        world->newEntity("Pipeline:PreFrame").set<ecs::SystemGroup>({1, false, 0.0f, 0.0f});
        world->newEntity("Pipeline:Early").set<ecs::SystemGroup>({2, false, 0.0f, 0.0f});
        world->newEntity("Pipeline:FixedUpdate").set<ecs::SystemGroup>({3, true, 0.0f, 0.02f});
        world->newEntity("Pipeline:Update").set<ecs::SystemGroup>({4, false, 0.0f, 0.0f});
        world->newEntity("Pipeline:PreRender").set<ecs::SystemGroup>({5, false, 0.0f, 0.0f});
        world->newEntity("Pipeline:Render").set<ecs::SystemGroup>({6, false, 0.0f, 0.0f});
        world->newEntity("Pipeline:PostRender").set<ecs::SystemGroup>({7, false, 0.0f, 0.0f});
        world->newEntity("Pipeline:Final").set<ecs::SystemGroup>({8, false, 0.0f, 0.0f});

        //lua_ = new LuaState();

        setupLuaEnvironment();
        loadLuaFile("/lua/engine");


        populateStartupData();

        //luabridge::LuaRef v2 = luabridge::getGlobal(lua_->L, "data");
        //populateStartupData();
        //auto x = v2["hfghfg"];

        modules.push_back(std::move(std::make_shared<IMGuiRender>(world.get(), this)));



        for(auto & m: modules) {
            m->registerModule();
        }

        for (auto& m : modules) {
            m->startup();
        }

        window_->onResize.AddLambda(
            [&](int w, int h)
            {
                setUint32ConfigValue("window", "width", w);
                setUint32ConfigValue("window", "height", h);
            });

        startTime = std::chrono::high_resolution_clock::now();
    }

    void EngineMain::shutdown()
    {
        RxCore::iVulkan()->WaitIdle();

        device_->clearQueues();
        RxCore::JobManager::instance().Shutdown();

        //world.reset();
        //renderCamera_.reset();

        world.reset();

        renderer_->shutdown();

        destroySemaphores();
        swapChain_.reset();
        renderer_.reset();
        window_.reset();
        device_.reset();
        window_.reset();
    }

    void EngineMain::update()
    {
        static float deltaAccumulated = 0.0f;

        OPTICK_FRAME("MainThread")
        //
        ///if (!renderCamera_) {
        //renderCamera_ = std::make_shared<RenderCamera>(scene_->getSceneCamera()->GetCamera());
        //}

        const auto last_clock = timer_;
        const auto time_now = std::chrono::high_resolution_clock::now();
        const auto fixed_frame_length = 1 / 60.f;

        const std::chrono::duration<float> delta_time = time_now - last_clock;
        totalElapsed_ = std::chrono::duration<float>(time_now - startTime).count();

        delta_ = delta_time.count();
        timer_ = time_now;
        deltaAccumulated += delta_;

        {
            OPTICK_EVENT("Window Updates")
            window_->Update(); // Collect the window events
        }

        if (deltaAccumulated > 10) {
            deltaAccumulated = 0;
        }
        world->setSingleton<EngineTime>({delta_, totalElapsed_});
        world->step(delta_);

        //        while (deltaAccumulated > fixed_frame_length) {
        //OPTICK_EVENT("ECS Progress");
        //deltaAccumulated -= fixed_frame_length;
        //}

        //updateMaterialGui();
        //updateEntityGui();
        //renderer_->updateGui();

        {
            OPTICK_EVENT("Check SwapChain")
            if (swapChain_->swapChainOutOfDate()) {
                replaceSwapChain();
            }
        }
        {
            OPTICK_EVENT("Scene Render")
            //std::vector<IRenderable *> renderables;
            //std::vector<IRenderProvider *> providers;

            // auto camera = scene_->getMainCamera();
            const auto current_extent = swapChain_->GetExtent();
#if 0
            scene_->GetRenderables(renderables);
            scene_->GetRenderProviders(providers);
#endif
            // Add the Rml Render data
            //renderables.push_back(rmlRender.get());
            //rmlRender->resetRender();
            {
                OPTICK_EVENT("Schedule PreRenders")
#if 0
                for (auto * renderable: providers) {
                    renderable->setup(scene_->getSceneCamera()->GetCamera());
                }
                for (auto * renderable: renderables) {
                    renderable->preRender(current_extent.width, current_extent.height);
                }
#endif
            }

            auto [next_swap_image_view, next_image_available, next_image_index] =
                swapChain_->AcquireNextImage();
            {
                OPTICK_EVENT("Actual Render")
                renderer_->render(
                    next_swap_image_view, current_extent, {next_image_available},
                    {vk::PipelineStageFlagBits::eColorAttachmentOutput},
                    submitCompleteSemaphores_[next_image_index]);
            }

            {
                // VkSwapchainKHR h = swapChain_->handle;
                OPTICK_GPU_FLIP(nullptr)
                OPTICK_CATEGORY("Present", Optick::Category::Rendering)
                swapChain_->PresentImage(
                    next_swap_image_view,
                    submitCompleteSemaphores_[next_image_index]);
            }
            {
                OPTICK_EVENT("Post Render")
                //                for (auto * r: renderables) {
                //                  r->postRender();
                //            }

                //  for (auto * renderable: providers) {
                //          renderable->teardown();
                //      }
            }
            RxCore::JobManager::instance().clean();
        }
    }

    void EngineMain::run()
    {
        while (!window_->ShouldClose()) {
            update();
        }
        // shutdown();
    }
#if 0
    void EngineMain::registerBaseModules()
    {
        //ecs_tracing_enable(3);

        ecs_measure_system_time(world_->get_world(), true);

        //        world_.

        world_->import<RxEngine::Mesh>();
        world_->import<RxEngine::Transforms>();
        world_->import<RxEngine::Render>();
    }
#endif
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

#if 0
    void EngineMain::populateMaterialPipeline(const std::string & name,
                                              luabridge::LuaRef & value)
    {
        auto vertexShader = value["vertexShader"].tostring();
        auto fragmentShader = value["fragmentShader"].tostring();

        auto depthTestEnable = value["depthTestEnable"].cast<bool>();
        auto depthWriteEnable = value["depthWriteEnable"].cast<bool>();

        Render::OpaquePipeline opl;
        Render::MaterialPipeline mpl;

        world_->entity().set<Render::OpaquePipeline>(Render::OpaquePipeline{
            {
                .depthTestEnable = depthTestEnable,
                .depthWriteEnable = depthWriteEnable,
            }
        });
    }
#endif

    void EngineMain::loadConfig()
    {
        mINI::INIFile file("engine.ini");
        file.read(iniData);
    }

    void EngineMain::saveConfig()
    {
        mINI::INIFile file("engine.ini");
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
        uint32_t defaultValue)
    {
        uint32_t v; // = defaultValue;
        try {
            v = std::stol(getConfigValue(section, entry));
        } catch (std::invalid_argument &) {
            v = defaultValue;
        }
        return v;
    }

    void EngineMain::setUint32ConfigValue(const std::string & section,
                                          const std::string & entry,
                                          uint32_t value)
    {
        setConfigValue(section, entry, std::to_string(value));
    }

    bool EngineMain::getBoolConfigValue(const std::string & section,
                                        const std::string & entry,
                                        bool defaultValue)
    {
        std::string v = getConfigValue(section, entry);
        if (v.empty()) {
            return defaultValue;
        }
        return v == "1";
    }

    void EngineMain::setBoolConfigValue(const std::string & section,
                                        const std::string & entry,
                                        bool value)
    {
        setConfigValue(section, entry, value ? "1" : "0");
    }
} // namespace RXEngine
