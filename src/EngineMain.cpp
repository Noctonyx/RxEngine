//
// Created by shane on 27/12/2020.
//

#include <vector>
#include <filesystem>
#include "Modules/Mesh.h"
#include "Modules/Render.h"
#include "EngineMain.hpp"
#include "Scene.h"
#include "Input/Mouse.hpp"
#include "Vulkan/Surface.hpp"
#include "Subsystems/SceneCamera.h"
#include <RmlUi/Core.h>
#include <RmlUi/Lua.h>
#include <imgui.h>
#include "Modules/Transforms.h"

//#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#if 0
#pragma warning( push )
#pragma warning(disable:4505)
#include "LuaBridge/LuaBridge.h"
#pragma warning( pop )

//#include "LuaBridge/LuaBridge.h"
#endif

namespace RxEngine
{
#if 0
    int LoadFileRequire(lua_State* L) {
        // use sol2 stack API to pull
        // "first argument"
        std::string path = sol::stack::get<std::string>(L, 1);
        path = "/lua/" + path;
        path = path + ".lua";
/*
        if (path == "a") {
            std::string script = R"(
            print("Hello from module land!")
            test = 123
            return "bananas"
        )";
        */
            auto script = RXAssets::vfs()->getStringFile(path);
            luaL_loadbuffer(L, script.data(), script.size(), path.c_str());
            // load "module", but don't run it
            //luaL_loadbuffer(L, script.data(), script.size(), path.c_str());
            // returning 1 object left on Lua stack:
            // a function that, when called, executes the script
            // (this is what lua_loadX/luaL_loadX functions return
            return 1;
        //}

      //  sol::stack::push(L, "This is not the module you're looking for!");
    //    return 1;
    }
#endif

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


        window_ = std::make_shared<Window>(width, height, "RX");

        device_ = std::make_unique<RxCore::Device>(window_->GetWindow());
        renderer_ = std::make_unique<Renderer>(device_->VkDevice(), world);

        auto surface = RxCore::Device::Context()->surface;

        swapChain_ = surface->CreateSwapChain();
        swapChain_->setSwapChainOutOfDate(true);

        window_->mouse = std::make_shared<Mouse>(window_.get());
        window_->keyboard = std::make_shared<Keyboard>(window_.get());

        renderer_->startup(swapChain_->imageFormat());

        materialManager_ = std::make_unique<MaterialManager>(device_.get());
        entityManager_ = std::make_unique<EntityManager>(materialManager_.get());

        /*const uint32_t g =*/
        entityManager_->addGroup(256, sizeof(RxAssets::MeshData::Vertex), false);
        //assert(g == 0);

        renderer_->setMaterialManager(materialManager_.get());
        renderer_->setEntityManager(entityManager_.get());

        timer_ = std::chrono::high_resolution_clock::now();

        rmlSystem = std::make_unique<RmlSystemInterface>(this);
        rmlFile = std::make_unique<RmlFileInterface>();
        rmlRender = std::make_unique<RmlRenderInterface>();

        Rml::SetSystemInterface(rmlSystem.get());
        Rml::SetFileInterface(rmlFile.get());
        Rml::SetRenderInterface(rmlRender.get());
        Rml::Initialise();

        //lua_ = new LuaState();

        setupLuaEnvironment();
        //luabridge::LuaRef v1 = luabridge::newTable(lua_->L);

        //luabridge::setGlobal(lua_->L, v1, "data");
//        luabridge::getGlobalNamespace(lua_->L)
  //          .beginNamespace("ind")
    //        .endNamespace();

        loadLuaFile("/lua/startup");
        populateStartupData();

        //luabridge::LuaRef v2 = luabridge::getGlobal(lua_->L, "data");
        //populateStartupData();
        //auto x = v2["hfghfg"];

#if 0
        lua_.open_libraries(sol::lib::base, sol::lib::package, sol::lib::table);

        lua_.clear_package_loaders( );
        lua_.add_package_loader(LoadFileRequire);

        lua_["print"] = [](sol::variadic_args va)
        {
            for(auto v: va) {

               auto x = v.get_type();
                if(x == sol::type::boolean) {
                    auto xx = v.get<bool>();
                    if(xx) {
                        spdlog::info("true");
                    } else {
                        spdlog::info("false");
                    }
                } else {
                    std::string w = v.get<std::string>();
                    spdlog::info(w);
                }
            }
        };

        /*
        sol::table loader = lua_["package"]["searchers"];
        loader.add(
            [](lua_State * L)
            {
                std::string path = sol::stack::get<std::string>(L,1);
                auto script = RXAssets::vfs()->getStringFile(path);
                luaL_loadbuffer(L, script.data(), script.size(), path.c_str());
                return 1;
            });
            */
/*
        lua_.add_package_loader([&](const std::string & filename){

            auto script = RXAssets::vfs()->getStringFile(filename);
            return lua_.load_buffer(script.data(), script.size(), filename);
        }, true);
*/
#endif
#if 0
        lua_.safe_script(R"(local x = require("startup")
            print(x)
            startup.test()
            )" /*sol::script_pass_on_error*/);
#endif
        Rml::LoadFontFace("/ui/fonts/TitilliumWeb-Regular.ttf");
        Rml::LoadFontFace("/ui/LatoLatin-Regular.ttf");
        Rml::LoadFontFace("/ui/fonts/Roboto-Regular.ttf");
        Rml::LoadFontFace("/ui/fonts/Roboto-Bold.ttf");
        Rml::Lua::Initialise();

        rmlRender->rendererInit(renderer_.get());

        window_->onResize.AddLambda(
            [&](int w, int h)
            {
                setUint32ConfigValue("window", "width", w);
                setUint32ConfigValue("window", "height", h);
                rmlRender->setDirty();
            });

        startTime = std::chrono::high_resolution_clock::now();
    }

    void EngineMain::setActiveScene(std::shared_ptr<Scene> scene)
    {
        if (scene_) {
            scene_->Shutdown();
        }
        if (scene) {
            //scene->setMaterialManager(materialManager_.get());
            //scene->setEntityManager(entityManager_.get());
            scene->Startup(renderer_.get());
        }
        scene_ = std::move(scene);
    }

    void EngineMain::shutdown()
    {
        RxCore::iVulkan()->WaitIdle();

        device_->clearQueues();
        RxCore::JobManager::instance().Shutdown();
        if (scene_) {
            scene_->Shutdown();
            //scene_->setMaterialManager(nullptr);
        }

        //world.reset();
        renderCamera_.reset();
        Rml::Shutdown();
        rmlRender.reset();
        rmlSystem.reset();
        rmlFile.reset();

        entityManager_.reset();
        materialManager_.reset();
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

        if (!renderCamera_) {
            renderCamera_ = std::make_shared<RenderCamera>(scene_->getSceneCamera()->GetCamera());
        }

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
        if (!scene_) {
            return;
        }
        //renderer_->setTextureBundle(scene_->getLoader()->getTextureBundle());

        {
            OPTICK_EVENT("Scene Update", Optick::Category::GameLogic)
            scene_->Update(delta_);
        }
        if (deltaAccumulated > 10) {
            deltaAccumulated = 0;
        }

        while (deltaAccumulated > fixed_frame_length) {
            OPTICK_EVENT("ECS Progress");
            world->step(fixed_frame_length);
            deltaAccumulated -= fixed_frame_length;
        }

        updateMaterialGui();
        updateEntityGui();
        renderer_->updateGui();

        {
            OPTICK_EVENT("Check SwapChain")
            if (swapChain_->swapChainOutOfDate()) {
                replaceSwapChain();
            }
        }
        {
            OPTICK_EVENT("Scene Render")
            std::vector<IRenderable *> renderables;
            std::vector<IRenderProvider *> providers;

            // auto camera = scene_->getMainCamera();
            const auto current_extent = swapChain_->GetExtent();

            scene_->GetRenderables(renderables);
            scene_->GetRenderProviders(providers);

            // Add the Rml Render data
            renderables.push_back(rmlRender.get());
            rmlRender->resetRender();
            {
                OPTICK_EVENT("Schedule PreRenders")

                for (auto * renderable: providers) {
                    renderable->setup(scene_->getSceneCamera()->GetCamera());
                }
                for (auto * renderable: renderables) {
                    renderable->preRender(current_extent.width, current_extent.height);
                }
            }

            auto [next_swap_image_view, next_image_available, next_image_index] =
                swapChain_->AcquireNextImage();
            {
                OPTICK_EVENT("Actual Render")
                renderer_->render(
                    renderCamera_, renderables,
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
                for (auto * r: renderables) {
                    r->postRender();
                }

                for (auto * renderable: providers) {
                    renderable->teardown();
                }
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

    MaterialManager * EngineMain::getMaterialManager() const
    {
        return materialManager_.get();
    }

    EntityManager * EngineMain::getEntityManager() const
    {
        return entityManager_.get();
    }
    
    void EngineMain::updateMaterialGui()
    {
        static bool show_material_window = getBoolConfigValue("editor", "materialWindow", false);
        static bool show_entity_window = getBoolConfigValue("editor", "entityWindow", false);

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Subsystems")) {
                if (ImGui::MenuItem("Materials", nullptr, show_material_window)) {
                    show_material_window = !show_material_window;
                    setBoolConfigValue("editor", "materialWindow", show_material_window);
                    //setConfigValue("editor", "materialWindow", showMaterialWindow ? "1" : "0");
                }
                if (ImGui::MenuItem("Entities", nullptr, show_entity_window)) {
                    show_entity_window = !show_entity_window;
                    setBoolConfigValue("editor", "entityWindow", show_entity_window);
                    //setConfigValue("editor", "materialWindow", showMaterialWindow ? "1" : "0");
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (show_material_window) {
            bool is_open = show_material_window;

            if (ImGui::Begin("Material Manager", &is_open)) {
                materialManager_->materialEditorGui();

            }
            ImGui::End();

            if (is_open != show_material_window) {
                show_material_window = is_open;
                setBoolConfigValue("editor", "materialWindow", show_material_window);
            }
        }
#if 0
        if (show_entity_window) {
            bool is_open = show_entity_window;

            if (ImGui::Begin("Entity Manager", &is_open)) {
                //entityManager_->entityEditorGui();

                ImGui::End();
            }

            if (is_open != show_entity_window) {
                show_entity_window = is_open;
                setBoolConfigValue("editor", "entityWindow", show_entity_window);
            }
        }
#endif
    }

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
