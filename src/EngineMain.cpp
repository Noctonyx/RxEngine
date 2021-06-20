////////////////////////////////////////////////////////////////////////////////
// MIT License
//
// Copyright (c) 2021.  Shane Hyde (shane@noctonyx.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

//
// Created by shane on 27/12/2020.
//

#include <vector>
#include <filesystem>
#include <memory>
#include <Modules/SwapChain/SwapChain.h>
#include <Modules/DynamicMesh/DynamicMesh.h>
#include "EngineMain.hpp"
#include "RXCore.h"
#include "RxECS.h"
#include "Modules/Renderer/Renderer.hpp"

#include "Modules/Module.h"
#include "Modules/Environment/Environment.h"
#include "Modules/ImGui/ImGuiRender.hpp"
#include "Modules/Lighting/Lighting.h"
#include "Modules/Mesh/Mesh.h"
#include "Modules/Stats/Stats.h"
#include "Modules/Prototypes/Prototypes.h"
#include "Modules/RmlUI/RmlUI.h"
#include "Events.h"
#include "Modules/RTSCamera/RTSCamera.h"
#include "Modules/SceneCamera/SceneCamera.h"
#include "Modules/Scene/SceneModule.h"
#include "Modules/StaticMesh/StaticMesh.h"
#include "Modules/Transforms/Transforms.h"
//#include "Modules/WorldObject/WorldObject.h"
#include "Vulkan/ThreadResources.h"
#include "Vulkan/Device.h"
#include "Window.hpp"
#include "Modules/SwapChain/SwapChain.h"
#include "FSM.h"

namespace RxEngine
{
    class StatsModule;

    void EngineMain::bootModules()
    {
        addModule<SwapChainModule>();

        //auto f = world->getModuleObject<SwapChainModule>()->getImageFormat();

        auto modId = world->createModule<Renderer>();
        modules.push_back(
            std::make_shared<Renderer>(
                device_.get(), world.get(),
                this, modId
            ));

        addModule<MaterialsModule>();
        addModule<RmlUiModule>();
        addModule<IMGuiRender>();
        addModule<SceneModule>();
//        addModule<TransformsModule>();
        addModule<StatsModule>();
        addModule<MeshModule>();
        addModule<StaticMeshModule>();
        addModule<DynamicMeshModule>();
        //addModule<WorldObjectModule>();
        addModule<PrototypesModule>();
        addModule<RTSCameraModule>();
        addModule<SceneCameraModule>();
        addModule<LightingModule>();

        lua->do_string("serpent = require('util/serpent'); data = require('util/data')");
        auto r = loadLuaFile("/lua/engine-data");
        if (!r.valid()) {
            return;
        }

        for (auto & m: modules) {
            world->pushModuleScope(m->getModuleId());
            m->loadData(lua->get<sol::table>("data").get<sol::table>("raw"));
            world->popModuleScope();
        }

        for (auto & m: modules) {
            world->pushModuleScope(m->getModuleId());
            m->startup();
            world->popModuleScope();
        }
    }

    void EngineMain::createSystems()
    {
        world->createSystem("Engine:OnResize")
             .withStream<WindowResize>()
             .inGroup("Pipeline:PostUpdate")
             .execute<WindowResize>(
                 [this](ecs::World *, const WindowResize * rs) {
                     setUint32ConfigValue("window", "width", rs->width);
                     setUint32ConfigValue("window", "height", rs->height);
                     return false;
                 }
             );

        world->createSystem("Engine:Clean")
             .inGroup("Pipeline:PostFrame")
             .execute(
                 [](ecs::World *) {
                     RxCore::JobManager::instance().clean();
                     RxCore::threadResources.freeAllResources();
                 }
             );

        world->createSystem("Engine:ECSMain")
             .inGroup("Pipeline:UpdateUi")
             .execute(
                 [this](ecs::World *) {
                     OPTICK_EVENT("Engine GUI")
                     updateEntityGui();
                 }
             );
    }

    void setupWorld(ecs::World * world, RxCore::Window * window)
    {
        auto g1 = world->newEntity("Pipeline:PreFrame").set<ecs::SystemGroup>(
            {1, false, 0.0f, 0.0f}
        );
        auto g2 = world->newEntity("Pipeline:Early").set<ecs::SystemGroup>({2, false, 0.0f, 0.0f});
        auto g3 = world->newEntity("Pipeline:FixedUpdate").set<ecs::SystemGroup>(
            {
                3, true, 0.0f, 0.02f
            }
        );
        auto g4 = world->newEntity("Pipeline:Update").set<ecs::SystemGroup>({4, false, 0.0f, 0.0f});
        auto g5 = world->newEntity("Pipeline:UpdateUi").set<ecs::SystemGroup>(
            {5, false, 0.0f, 0.0f}
        );
        auto g6 = world->newEntity("Pipeline:PostUpdate").set<ecs::SystemGroup>(
            {
                6, false, 0.0f, 0.0f
            }
        );
        auto g7 = world->newEntity("Pipeline:PreRender").set<ecs::SystemGroup>(
            {
                7, false, 0.0f, 0.0f
            }
        );
        auto g8 = world->newEntity("Pipeline:Render").set<ecs::SystemGroup>({8, false, 0.0f, 0.0f});
        auto g9 = world->newEntity("Pipeline:PostRender").set<ecs::SystemGroup>(
            {
                9, false, 0.0f, 0.0f
            }
        );
        auto g10 = world->newEntity("Pipeline:PostFrame").set<ecs::SystemGroup>(
            {
                10, false, 0.0f, 0.0f
            }
        );

        g1.update<ecs::SystemGroup>(
            [=](auto * sg1) {
                sg1->onBegin = []() {
                    OPTICK_PUSH("Pipeline:PreFrame")
                };
                sg1->onEnd = []() {
                    OPTICK_POP()
                };

            }
        );
        g2.update<ecs::SystemGroup>(
            [=](auto * sg2) {
                sg2->onBegin = []() {
                    OPTICK_PUSH("Pipeline:Early")
                };
                sg2->onEnd = []() {
                    OPTICK_POP()
                };

            }
        );
        g3.update<ecs::SystemGroup>(
            [=](auto * sg3) {
                sg3->onBegin = []() {
                    OPTICK_PUSH("Pipeline:FixedUpdate")
                };
                sg3->onEnd = []() {
                    OPTICK_POP()
                };

            }
        );
        g4.update<ecs::SystemGroup>(
            [=](auto * sg4) {
                sg4->onBegin = []() {
                    OPTICK_PUSH("Pipeline:Update")
                };
                sg4->onEnd = []() {
                    OPTICK_POP()
                };

            }
        );
        g5.update<ecs::SystemGroup>(
            [=](auto * sg5) {
                sg5->onBegin = []() {
                    OPTICK_PUSH("Pipeline:UpdateUi")
                };
                sg5->onEnd = []() {
                    OPTICK_POP()
                };

            }
        );
        g6.update<ecs::SystemGroup>(
            [=](auto * sg6) {
                sg6->onBegin = []() {
                    OPTICK_PUSH("Pipeline:PostUpdate")
                };
                sg6->onEnd = []() {
                    OPTICK_POP()
                };

            }
        );
        g7.update<ecs::SystemGroup>(
            [=](auto * sg7) {
                sg7->onBegin = []() {
                    OPTICK_PUSH("Pipeline:PreRender")
                };
                sg7->onEnd = []() {
                    OPTICK_POP()
                };

            }
        );
        g8.update<ecs::SystemGroup>(
            [=](auto * sg8) {
                sg8->onBegin = []() {
                    OPTICK_PUSH("Pipeline:Render")
                };
                sg8->onEnd = []() {
                    OPTICK_POP()
                };

            }
        );
        g9.update<ecs::SystemGroup>(
            [=](auto * sg9) {
                sg9->onBegin = []() {
                    OPTICK_PUSH("Pipeline:PostRender")
                };
                sg9->onEnd = []() {
                    OPTICK_POP()
                };

            }
        );
        g10.update<ecs::SystemGroup>(
            [=](auto * sg10) {
                sg10->onBegin = []() {
                    OPTICK_PUSH("Pipeline:PostFrame")
                };
                sg10->onEnd = []() {
                    OPTICK_POP()
                };

            }
        );

        world->setSingleton<WindowDetails>(
            {
                window, window->getWidth(), window->getHeight()
            }
        );
        //window_->setWorld(world.get());
    }

    bool EngineMain::startup(const char * windowTitle)
    {
        loadConfig();

        uint32_t width = getUint32ConfigValue("window", "width", 1280);
        uint32_t height = getUint32ConfigValue("window", "height", 768);

        if (width <= 0) {
            width = 800;
        }
        if (height <= 0) {
            height = 600;
        }

        world = std::make_unique<ecs::World>();
        world->setJobInterface(&jobAdapter);

        RxCore::Events::startup();

        window_ = std::make_unique<RxCore::Window>(width, height, windowTitle);

        device_ = decltype(device_){
            new RxCore::Device(window_->GetWindow()), [](RxCore::Device * d) {
                delete d;
            }
        };

        // auto surface = RxCore::Device::Context()->surface;

        RxCore::JobManager::instance().freeAllResourcesFunction = []() {
            RxCore::threadResources.freeAllResources();
        };

        RxCore::JobManager::instance().freeResourcesFunction = []() {
            RxCore::threadResources.freeUnused();
        };

        auto d = device_.get();
        RxCore::JobManager::instance().initFunction = [d]() {
            RxCore::setThreadDevice(d);
            //RxCore::threadResources.device = d;
        };
        RxCore::threadResources.setDevice(d);

        RxCore::JobManager::instance().startup();

        timer_ = std::chrono::high_resolution_clock::now();

        setupWorld(world.get(), window_.get());

        lua = new sol::state();

        setupLuaEnvironment();

        bootModules();
        createSystems();

        world->set<ComponentGui>(world->getComponentId<ecs::Name>(), {.editor = ecsNameGui});
        world->set<ComponentGui>(
            world->getComponentId<ecs::SystemGroup>(),
            {.editor = ecsSystemGroupGui}
        );
        world->set<ComponentGui>(
            world->getComponentId<WindowDetails>(),
            {.editor = ecsWindowDetailsGui}
        );
        world->set<ComponentGui>(world->getComponentId<EngineTime>(), {.editor = ecsEngineTimeGui});
        world->set<ComponentGui>(
            world->getComponentId<ecs::StreamComponent>(),
            {.editor = ecsStreamComponentGui}
        );
        world->set<ComponentGui>(world->getComponentId<ecs::System>(), {.editor = ecsSystemGui});
        world->set<ComponentGui>(
            world->getComponentId<ecs::Component>(),
            {.editor = ecsComponentGui}
        );
        world->set<ComponentGui>(world->getComponentId<FrameStats>(), {.editor = frameStatsGui});

        for (auto & m: modules) {
            m->enable();
        }

        return true;
    }

    void EngineMain::shutdown()
    {
        std::vector<std::shared_ptr<Module>> m1;

        std::ranges::reverse(modules.begin(), modules.end());
        for (auto & m: modules) {
            m->disable();
        }

        device_->WaitIdle();

        device_->clearQueues();
        RxCore::JobManager::instance().Shutdown();

        for (auto & m: userModules) {
            m->shutdown();
        }
        for (auto & m: modules) {
            m->shutdown();
        }
        userModules.clear();
        modules.clear();

        world.reset();

        delete lua;

        window_.reset();
        device_.reset();
        RxAssets::vfs()->shutdown();
        RxCore::Events::shutdown();
    }

    EInputMod getKeyMod()
    {
        SDL_Keymod m = RxCore::Events::getKeyboardMods();
        EInputMod mod = EInputMod_None;
        if (m & KMOD_CTRL) {
            mod = mod | EInputMod_Control;
        }
        if (m & KMOD_SHIFT) {
            mod = mod | EInputMod_Shift;
        }
        if (m & KMOD_ALT) {
            mod = mod | EInputMod_Alt;
        }

        return mod;
    }

    EKey mapSDLToEKey(SDL_Keycode kc)
    {
        switch (kc) {
        case SDLK_ESCAPE:
            return EKey::Escape;
        case SDLK_BACKSPACE:
            return EKey::Backspace;
        case SDLK_TAB:
            return EKey::Tab;
        case SDLK_SPACE:
            return EKey::Space;
        case SDLK_LEFTPAREN:
            return EKey::LeftBracket;
        case SDLK_RIGHTPAREN:
            return EKey::RightBracket;
        case SDLK_COMMA:
            return EKey::Comma;
        case SDLK_MINUS:
            return EKey::Minus;
        case SDLK_PERIOD:
            return EKey::Period;
        case SDLK_SLASH:
            return EKey::Slash;
        case SDLK_0:
            return EKey::_0;
        case SDLK_1:
            return EKey::_1;
        case SDLK_2:
            return EKey::_2;
        case SDLK_3:
            return EKey::_3;
        case SDLK_4:
            return EKey::_4;
        case SDLK_5:
            return EKey::_5;
        case SDLK_6:
            return EKey::_6;
        case SDLK_7:
            return EKey::_7;
        case SDLK_8:
            return EKey::_8;
        case SDLK_9:
            return EKey::_9;
        case SDLK_SEMICOLON:
            return EKey::Semicolon;
        case SDLK_EQUALS:
            return EKey::Equal;
        case SDLK_LEFTBRACKET:
            return EKey::LeftBracket;
        case SDLK_BACKSLASH:
            return EKey::Backslash;
        case SDLK_RIGHTBRACKET:
            return EKey::RightBracket;
        case SDLK_a:
            return EKey::A;
        case SDLK_b:
            return EKey::B;
        case SDLK_c:
            return EKey::C;
        case SDLK_d:
            return EKey::D;
        case SDLK_e:
            return EKey::E;
        case SDLK_f:
            return EKey::F;
        case SDLK_g:
            return EKey::G;
        case SDLK_h:
            return EKey::H;
        case SDLK_i:
            return EKey::I;
        case SDLK_j:
            return EKey::J;
        case SDLK_k:
            return EKey::K;
        case SDLK_l:
            return EKey::L;
        case SDLK_m:
            return EKey::M;
        case SDLK_n:
            return EKey::N;
        case SDLK_o:
            return EKey::O;
        case SDLK_p:
            return EKey::P;
        case SDLK_q:
            return EKey::Q;
        case SDLK_r:
            return EKey::R;
        case SDLK_s:
            return EKey::S;
        case SDLK_t:
            return EKey::T;
        case SDLK_u:
            return EKey::U;
        case SDLK_v:
            return EKey::V;
        case SDLK_w:
            return EKey::W;
        case SDLK_x:
            return EKey::X;
        case SDLK_y:
            return EKey::Y;
        case SDLK_z:
            return EKey::Z;
        case SDLK_F1:
            return EKey::F1;
        case SDLK_F2:
            return EKey::F2;
        case SDLK_F3:
            return EKey::F3;
        case SDLK_F4:
            return EKey::F4;
        case SDLK_F5:
            return EKey::F5;
        case SDLK_F6:
            return EKey::F6;
        case SDLK_F7:
            return EKey::F7;
        case SDLK_F8:
            return EKey::F8;
        case SDLK_F9:
            return EKey::F9;
        case SDLK_F10:
            return EKey::F10;
        case SDLK_F11:
            return EKey::F11;
        case SDLK_F12:
            return EKey::F12;
        case SDLK_INSERT:
            return EKey::Insert;
        case SDLK_HOME:
            return EKey::Home;
        case SDLK_PAGEUP:
            return EKey::PageUp;
        case SDLK_DELETE:
            return EKey::Delete;
        case SDLK_END:
            return EKey::End;
        case SDLK_PAGEDOWN:
            return EKey::PageDown;
        case SDLK_RIGHT:
            return EKey::Right;
        case SDLK_LEFT:
            return EKey::Left;
        case SDLK_DOWN:
            return EKey::Down;
        case SDLK_UP:
            return EKey::Up;

        }
        return EKey::Unknown;
    }

    void EngineMain::handleEvents()
    {
        RxCore::Events::pollEvents(
            [this](SDL_Event * ev) {
                switch (ev->type) {
                case SDL_QUIT:
                    shouldQuit = true;
                    break;

                case SDL_KEYUP:
                case SDL_KEYDOWN: {
                    auto k = mapSDLToEKey(ev->key.keysym.sym);
                    world->getStream<KeyboardKey>()->add(
                        KeyboardKey
                            {
                                k,
                                ev->key.state == SDL_PRESSED
                                ? EInputAction::Press
                                : EInputAction::Release,
                                getKeyMod()
                            }
                    );
                }
                    if (ev->key.state == SDL_PRESSED) {
                        world->getStream<KeyboardChar>()->add<KeyboardChar>(
                            {
                                static_cast<char>(ev->key.keysym.sym)
                            }
                        );
                    }
                    break;
                case SDL_MOUSEMOTION: {
                    MousePosition pos{
                        static_cast<float>(ev->motion.x),
                        static_cast<float>(ev->motion.y),
                        static_cast<float>(ev->motion.xrel),
                        static_cast<float>(ev->motion.yrel),
                        getKeyMod(),
                        capturedMouse
                    };
                    world->getStream<MousePosition>()->add(pos);
                }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP: {
                    world->getStream<MouseButton>()->add(
                        MouseButton
                            {
                                ev->button.button - 1,
                                ev->button.state == SDL_PRESSED,
                                getKeyMod()
                            }
                    );
                }
                    break;
                case SDL_MOUSEWHEEL: {
                    world->getStream<MouseScroll>()->add(
                        MouseScroll
                            {
                                static_cast<float>(ev->wheel.y),
                                getKeyMod()
                            }
                    );
                }
                    break;
                case SDL_WINDOWEVENT: {
                    switch (ev->window.event) {
                    case SDL_WINDOWEVENT_RESIZED:
                        WindowResize res{
                            static_cast<uint32_t>(ev->window.data1),
                            static_cast<uint32_t>(ev->window.data2)
                        };
                        world->getStream<WindowResize>()->add(res);
                        auto wd = world->getSingletonUpdate<WindowDetails>();
                        wd->width = ev->window.data1;
                        wd->height = ev->window.data2;
                        break;
                    }
                }
                    break;
                }
            }
        );
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
            OPTICK_EVENT("Handle Events")
            handleEvents();
        }

        if (RxAssets::vfs()->hasChanged()) {
            RxAssets::vfs()->scan();
        }

        world->setSingleton<EngineTime>({delta_, totalElapsed_});
        {
            OPTICK_EVENT("Step World")
            world->step(delta_);
        }
    }

    void EngineMain::run()
    {
        startTime = std::chrono::high_resolution_clock::now();

        while (!shouldQuit) {
            update();
        }
    }

    size_t EngineMain::getUniformBufferAlignment(size_t size) const
    {
        return device_->getUniformBufferAlignment(size);
    }

    std::shared_ptr<RxCore::Buffer> EngineMain::createUniformBuffer(size_t size) const
    {
        return device_->createBuffer(
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU, size
        );
    }

    std::shared_ptr<RxCore::Buffer> EngineMain::createStorageBuffer(size_t size) const
    {
        return device_->createBuffer(
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU, size
        );
    }

    void EngineMain::captureMouse(bool enable)
    {
        capturedMouse = enable;
        window_->hideCursor(enable);
    }

    void EngineMain::loadDataFile(const std::filesystem::path & path)
    {
        sol::state loaderState;

        createDataLoaderEnvironment(loaderState);
        //loaderState

        loadDataFile(loaderState, path);

        sol::table t = loaderState.get<sol::table>("data").get<sol::table>("raw");
        loadDataToModules(t);
    }

    void EngineMain::loadDataToModules(sol::table & dataTable)
    {
        for (auto & m: modules) {
            world->pushModuleScope(m->getModuleId());
            m->loadData(dataTable);
            world->popModuleScope();
        }
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

#if 0
    void EngineMain::addInitConfigFile(const std::string & config)
    {
        configFiles.push_back(config);
    }
#endif

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

    void EngineMain::startRuntime()
    {
        lua->do_string("serpent = require('util/serpent');");

        registerEngineTypes(*lua);

        for (auto & m: modules) {
            m->registerRuntime(*lua);
        }
        for (auto & m: userModules) {
            m->registerRuntime(*lua);
        }
    }

    void EngineMain::registerEngineTypes(sol::state &)
    {
        lua->new_usertype<FSM>("FSM", "getCurrentState", &FSM::getCurrentState);
    }

    ecs::JobInterface::JobHandle RxJobAdaptor::create(std::function<uint32_t()> f)
    {
        return RxCore::CreateJob<uint32_t>(f);
    }

    void RxJobAdaptor::schedule(ecs::JobInterface::JobHandle job_handle)
    {
        auto x = std::static_pointer_cast<RxCore::Job<uint32_t>>(job_handle);
        x->schedule();
    }

    bool RxJobAdaptor::isComplete(ecs::JobInterface::JobHandle job_handle) const
    {
        auto x = std::static_pointer_cast<RxCore::Job<uint32_t>>(job_handle);
        return x->isCompleted();
    }

    void RxJobAdaptor::awaitCompletion(ecs::JobInterface::JobHandle job_handle)
    {
        auto x = std::static_pointer_cast<RxCore::Job<uint32_t>>(job_handle);
        x->waitComplete();
    }

    uint32_t RxJobAdaptor::getJobResult(ecs::JobInterface::JobHandle job_handle)
    {
        auto x = std::static_pointer_cast<RxCore::Job<uint32_t>>(job_handle);
        return x->result;
    }
} // namespace RXEngine
