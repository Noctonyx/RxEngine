//
// Created by shane on 27/12/2020.
//

#ifndef RX_ENGINEMAIN_HPP
#define RX_ENGINEMAIN_HPP

#include <vector>
#include <memory>
#include <chrono>
#include <RXCore.h>
#include "Modules/Renderer/Renderer.hpp"
//#include <Rendering/EntityManager.h>

//#include "Delegates.hpp"
//#include "Rendering/MaterialManager.h"

//#include "UI/RmlFileInterface.h"
//#include "UI/RmlSystemInterface.h"
//#include "UI/RmlRenderInterface.h"
#pragma warning(disable: 4706)
#include "ini.h"
#include "RxECS.h"
#include "Vfs.h"
#include "sol/sol.hpp"
#include "Window.hpp"
#include "Modules/Module.h"
#include "imgui.h"
//#include "LuaBridge/detail/LuaRef.h"

//#include <sol/sol.hpp>


namespace RxAssets
{
    class Loader;
}

namespace RxCore
{
    class SwapChain;
}

namespace RxEngine
{
    class Subsystem;
    class Window;
    class Scene;

    struct EngineTime
    {
        float delta;
        float totalElapsed;
    };

    struct ComponentGui
    {
        std::function<void(ecs::World *, void *)> editor;
    };
    
    struct RxJobAdaptor : ecs::JobInterface
    {
        JobHandle create(std::function<void()> f) override {
            return RxCore::CreateJob<void>(f);
        }

        void schedule(JobHandle job_handle) override {
            auto x = std::static_pointer_cast<RxCore::Job<void>>(job_handle);
            x->schedule();
        }

        bool isComplete(JobHandle job_handle) const override {
            auto x = std::static_pointer_cast<RxCore::Job<void>>(job_handle);
            return x->isCompleted();
        }

        void awaitCompletion(JobHandle job_handle) override {
            auto x = std::static_pointer_cast<RxCore::Job<void>>(job_handle);
            x->waitComplete();
        }
    };

    class EngineMain
    {
    public:
        EngineMain()
        {
            // world_ = std::make_unique<flecs::world>();
        };

        ~EngineMain()
        {
            shutdown();
        }

        void bootModules();
        void createSystems();
        void setupWorld();
        void startup(const char * windowTitle);
        void loadModules();

        //void setActiveScene(std::shared_ptr<Scene> scene);
        void run();
        void update();
        void shutdown();

        //void setLoader(RXAssets::Loader * );

        float getTotalElapsed() const
        {
            return totalElapsed_;
        }

        Window * getWindow() const
        {
            return window_.get();
        }

        void loadConfig();
        void saveConfig();

        void setConfigValue(const std::string & section,
                            const std::string & entry,
                            const std::string & value);
        void setBoolConfigValue(const std::string & section, const std::string & entry, bool value);
        void setUint32ConfigValue(const std::string & section,
                                  const std::string & entry,
                                  uint32_t value);
        std::string getConfigValue(const std::string & section, const std::string & entry);
        bool getBoolConfigValue(const std::string & section,
                                const std::string & entry,
                                bool defaultValue);
        uint32_t getUint32ConfigValue(const std::string & section,
                                      const std::string & entry,
                                      uint32_t defaultValue);


        void addInitConfigFile(const std::string & config);

        [[nodiscard]] ecs::World * getWorld() const
        {
            return world.get();
        }

        template <class T>
        void addModule();

        [[nodiscard]] size_t getUniformBufferAlignment(size_t size) const;
        [[nodiscard]] std::shared_ptr<RxCore::Buffer> createUniformBuffer(size_t size) const;
        [[nodiscard]] std::shared_ptr<RxCore::Buffer> createStorageBuffer(size_t size) const;

        void addUserModule(std::shared_ptr<Module> module);

    protected:
        void replaceSwapChain();
        void createSemaphores(uint32_t semaphoreCount);
        void destroySemaphores();

        void createMaterialTexture(std::string textureName, sol::table details);
        void loadLuaFile(const std::filesystem::path & file);
        void setupLuaEnvironment();

        void ecsMainMenu(bool & show_entity_window,
                         bool & show_systems_window,
                         bool & show_singletons_window);
        void ecsInspectorShowTableDetails(ecs::EntityHandle & selectedEntity,
                                          ecs::Table * table) const;
        void ecsInspectorIsAEntity(ecs::EntityHandle entity, ecs::EntityHandle & selectedEntity);
        void ecsInspectorEntityComponents(ecs::EntityHandle entity,
                                          ecs::EntityHandle & selectedEntity);
        void ecsInspectorEntityWindow(bool & show_entity_window);
        void ecsSingletonsWindow(bool & show_singletons_window);
        void showSystemsGui(bool & showWindow);
        void updateEntityGui();


    private:
        //std::vector<std::unique_ptr<Subsystem>> subsystems_;
        std::unique_ptr<Window> window_;
        //std::unique_ptr<Renderer> renderer_;
        std::unique_ptr<RxCore::Device> device_;
        std::unique_ptr<RxCore::SwapChain> swapChain_;
        //std::unique_ptr<MaterialManager> materialManager_;
        //std::unique_ptr<EntityManager> entityManager_;

        std::vector<std::shared_ptr<Module>> modules;
        std::vector<std::shared_ptr<Module>> userModules;

        std::vector<vk::Semaphore> submitCompleteSemaphores_;
        std::chrono::time_point<std::chrono::steady_clock> timer_;

        std::vector<std::string> configFiles;

        std::chrono::time_point<std::chrono::steady_clock> startTime;
        float delta_{};
        float totalElapsed_{};

        sol::state * lua = nullptr;

        mINI::INIStructure iniData;

        std::unique_ptr<ecs::World> world;
        RxJobAdaptor jobAdapter;
    };

    template <class T>
    void EngineMain::addModule()
    {
        modules.push_back(std::make_shared<T>(world.get(), this));
    }

    inline void EngineMain::loadLuaFile(const std::filesystem::path & file)
    {
        auto path = file;
        if (!path.has_extension()) {
            path.replace_extension(".lua");
        }


        const std::string script = RxAssets::vfs()->getStringFile(path);
        lua->script(script, path.generic_string());
        //. load_buffer(script.c_str(), script.length(), path.generic_string().c_str());
#if 0
        if (r == 0) {
            if (lua. (L, 0, LUA_MULTRET, 0) != 0) {
                const auto error_string = lua_tostring(L, -1);
                throw std::runtime_error(error_string ? error_string : "Unknown lua error");
            }
        }
        else {
            spdlog::error("Error in {} - {}", file.generic_string().c_str(), lua_tostring(L, -1));
            throw std::runtime_error(lua_tostring(L, -1));
        }
#endif
    }

    void ecsNameGui(ecs::World * world, void * ptr);
    void ecsComponentGui(ecs::World * world, void * ptr);
    void ecsSystemGroupGui(ecs::World * world, void * ptr);
    void ecsWindowDetailsGui(ecs::World * world, void * ptr);
    void ecsEngineTimeGui(ecs::World * world, void * ptr);
    void ecsSystemGui(ecs::World * world, void * ptr);
    void ecsStreamComponentGui(ecs::World * world, void * ptr);
    void frameStatsGui(ecs::World * world, void * ptr);
}
#endif //RX_ENGINEMAIN_HPP
