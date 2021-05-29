#pragma once
#include <functional>
#include "RxECS.h"

namespace RxEngine
{
    struct FSM;
    class EngineMain;

    struct BaseFSMState
    {
        //std::string name;
    public:
        virtual ~BaseFSMState() = default;
        virtual void onEntry(ecs::World * world, EngineMain * engine) { }
        virtual void onExit(ecs::World * world, EngineMain * engine) {}
        virtual void step(FSM * fsm, ecs::World * world, EngineMain * engine) = 0;

        virtual void onTransition(std::string_view target,
                                  ecs::World * world,
                                  EngineMain * engine) {}
        virtual std::string getName() = 0;
    };

#if 0
    struct FSMState
    {
        std::string name;

        std::function<void(ecs::World *, EngineMain *)> onEntry = [](ecs::World *, EngineMain *) {};
        std::function<void(ecs::World *, EngineMain *)> onExit = [](ecs::World *, EngineMain *) {};
        std::function<void(ecs::World *, EngineMain *)> update = [](ecs::World *, EngineMain *) {};

        std::unordered_map<const std::string, std::function<void(ecs::World *, EngineMain *)>>
        onTransition{};
    };
#endif
    struct FSM
    {
        BaseFSMState * currentState = nullptr;
        std::string nextStateName = "";

        std::unordered_map<std::string, std::unique_ptr<BaseFSMState>> states;

        void step(ecs::World *, EngineMain *);
        void addState(std::unique_ptr<BaseFSMState> && state);

        void start(const std::string & name, ecs::World * w, EngineMain * e);
    };
}
