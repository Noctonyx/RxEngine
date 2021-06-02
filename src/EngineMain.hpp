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
    class Window;
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
        JobHandle create(std::function<void()> f) override
        {
            return RxCore::CreateJob<void>(f);
        }

        void schedule(JobHandle job_handle) override
        {
            auto x = std::static_pointer_cast<RxCore::Job<void>>(job_handle);
            x->schedule();
        }

        bool isComplete(JobHandle job_handle) const override
        {
            auto x = std::static_pointer_cast<RxCore::Job<void>>(job_handle);
            return x->isCompleted();
        }

        void awaitCompletion(JobHandle job_handle) override
        {
            auto x = std::static_pointer_cast<RxCore::Job<void>>(job_handle);
            x->waitComplete();
        }
    };

    struct WindowDetails
    {
        RxCore::Window* window;

        uint32_t width;
        uint32_t height;
    };

    struct WindowResize
    {
        uint32_t width;
        uint32_t height;
    };

    enum class EKey : int16_t
    {
        Unknown = -1,
        Space = 32,
        Apostrophe = 39,
        Comma = 44,
        Minus = 45,
        Period = 46,
        Slash = 47,
        _0 = 48,
        _1 = 49,
        _2 = 50,
        _3 = 51,
        _4 = 52,
        _5 = 53,
        _6 = 54,
        _7 = 55,
        _8 = 56,
        _9 = 57,
        Semicolon = 59,
        Equal = 61,
        A = 65,
        B = 66,
        C = 67,
        D = 68,
        E = 69,
        F = 70,
        G = 71,
        H = 72,
        I = 73,
        J = 74,
        K = 75,
        L = 76,
        M = 77,
        N = 78,
        O = 79,
        P = 80,
        Q = 81,
        R = 82,
        S = 83,
        T = 84,
        U = 85,
        V = 86,
        W = 87,
        X = 88,
        Y = 89,
        Z = 90,
        LeftBracket = 91,
        Backslash = 92,
        RightBracket = 93,
        GraveAccent = 96,
        World1 = 161,
        World2 = 162,
        Escape = 256,
        Enter = 257,
        Tab = 258,
        Backspace = 259,
        Insert = 260,
        Delete = 261,
        Right = 262,
        Left = 263,
        Down = 264,
        Up = 265,
        PageUp = 266,
        PageDown = 267,
        Home = 268,
        End = 269,
        CapsLock = 280,
        ScrollLock = 281,
        NumLock = 282,
        PrintScreen = 283,
        Pause = 284,
        F1 = 290,
        F2 = 291,
        F3 = 292,
        F4 = 293,
        F5 = 294,
        F6 = 295,
        F7 = 296,
        F8 = 297,
        F9 = 298,
        F10 = 299,
        F11 = 300,
        F12 = 301,
        F13 = 302,
        F14 = 303,
        F15 = 304,
        F16 = 305,
        F17 = 306,
        F18 = 307,
        F19 = 308,
        F20 = 309,
        F21 = 310,
        F22 = 311,
        F23 = 312,
        F24 = 313,
        F25 = 314,
        Numpad0 = 320,
        Numpad1 = 321,
        Numpad2 = 322,
        Numpad3 = 323,
        Numpad4 = 324,
        Numpad5 = 325,
        Numpad6 = 326,
        Numpad7 = 327,
        Numpad8 = 328,
        Numpad9 = 329,
        NumpadDecimal = 330,
        NumpadDivide = 331,
        NumpadMultiply = 332,
        NumpadSubtract = 333,
        NumpadAdd = 334,
        NumpadEnter = 335,
        NumpadEqual = 336,
        ShiftLeft = 340,
        ControlLeft = 341,
        AltLeft = 342,
        SuperLeft = 343,
        ShiftRight = 344,
        ControlRight = 345,
        AltRight = 346,
        SuperRight = 347,
        Menu = 348,
        First = Space,
        Last = Menu
    };

    enum class EInputAction : int32_t
    {
        Release = 0,
        Press = 1,
        Repeat = 2
    };

    enum EInputMod 
    {
        EInputMod_None = 0,
        EInputMod_Shift = 1,
        EInputMod_Control = 2,
        EInputMod_Alt = 4,
        EInputMod_Super = 8
    };

    inline EInputMod operator|(EInputMod a, EInputMod b)
    {
        return static_cast<EInputMod>(static_cast<int>(a) | static_cast<int>(b));
    }

    struct KeyboardChar
    {
        char c;
    };

    struct KeyboardKey
    {
        EKey key;
        EInputAction action;
        EInputMod mods;
    };

    enum class ECursorHotspot : uint8_t
    {
        UpperLeft,
        UpperRight,
        BottomLeft,
        BottomRight,
        Centered
    };


    struct MousePosition
    {
        float x;
        float y;

        float deltaX;
        float deltaY;

        EInputMod mods;
        bool captured;
    };

    struct MouseButton
    {
        int32_t button;
        bool pressed;
        EInputMod mods;
    };

    struct MouseScroll
    {
        float y_offset;
        EInputMod mods;
    };

    struct MouseStatus
    {
        bool button1;
        bool button2;
        bool button3;

        float mouseX;
        float mouseY;

        float deltaMouseX;
        float deltaMouseY;
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
        void setupWorld() const;
        bool loadLuaFiles() const;
        bool startup(const char * windowTitle);
        void loadModules();

        //void setActiveScene(std::shared_ptr<Scene> scene);
        void run();
        void handleEvents();
        void update();
        void shutdown();

        //void setLoader(RXAssets::Loader * );

        float getTotalElapsed() const
        {
            return totalElapsed_;
        }

        RxCore::Window * getWindow() const
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

        void captureMouse(bool enable);

        [[nodiscard]] sol::state * getLua() const
        {
            return lua;
        }

        [[nodiscard]] RxCore::Device * getDevice() const
        {
            return device_.get();
        }

    protected:
        void replaceSwapChain();
        void createSemaphores(uint32_t semaphoreCount);
        void destroySemaphores();

        void createMaterialTexture(std::string textureName, sol::table details);
        sol::protected_function_result loadLuaFile(const std::filesystem::path & file) const;
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
        std::unique_ptr<RxCore::Window> window_;
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

        bool shouldQuit = false;
        bool capturedMouse = false;
    };

    template <class T>
    void EngineMain::addModule()
    {
        auto modId = world->createModule<T>();
        //auto name = ecs::World::trimName(typeid(std::remove_reference_t<T>).name());
        //auto modId = world->newEntity(name.c_str()).add<ecs::Module>();
        modules.push_back(std::make_shared<T>(world.get(), this, modId));
    }

    inline sol::protected_function_result EngineMain::loadLuaFile(const std::filesystem::path & file) const
    {
        auto path = file;
        if (!path.has_extension()) {
            path.replace_extension(".lua");
        }

        const std::string script = RxAssets::vfs()->getAssetAsString(path);
        auto result = lua->safe_script(script, path.generic_string());

        if (!result.valid()) {
            sol::error err = result;
            std::string what = err.what();
            spdlog::critical("Lua error - {0}", err.what());
        }

        return result;
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
