//
// Created by shane on 3/04/2020.
//

#ifndef AMX_SUBSYSTEM_HPP
#define AMX_SUBSYSTEM_HPP
#include <optional>
//#include "Scene.h"
#include "Vulkan/CommandBuffer.hpp"
//#include "Rendering/Renderer.hpp"

namespace RxEngine
{
    class Scene;
    class Subsystem;

    enum class Sequences: uint8_t
    {
        //Startup = 0,
        RendererInit = 1,
        PreUpdate = 2,
        FixedUpdate = 3,
        Update = 4,
        LateUpdate = 5,
        UpdateGui = 6,
        Render = 7,
        //Shutdown = 8
    };

    struct SubsystemPri
    {
        Subsystem * s;
        int16_t pri;
    };

    class Subsystem
    {
    public:
        Subsystem() = default;

        virtual ~Subsystem() = default;

        virtual int16_t GetCallPriority(Sequences)
        {
            return 0;
        }

        virtual void PreUpdate() {}
        virtual void FixedUpdate() {}
        virtual void Update(float delta) {}
        virtual void LateUpdate() {}
        virtual void UpdateGui() {}

        virtual void addedToScene(Scene * scene,
                                  std::vector<std::shared_ptr<Subsystem>> & subsystems)
        {
            scene_ = scene;
        }

        virtual void removedFromScene()
        {
            scene_ = nullptr;
        }

        virtual void onSubsystemAdded(std::shared_ptr<Subsystem> subsys) {}
        virtual void onSubsystemRemoved(const std::shared_ptr<Subsystem> & subsys) {}

        Scene * scene_;
    };
} // namespace RXCore

#endif // AMX_SUBSYSTEM_HPP
