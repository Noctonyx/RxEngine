#include "FSM.h"

namespace RxEngine
{
    void FSM::step(ecs::World* w, EngineMain* e)
    {
        if (currentState) {
            currentState->step(this, w, e);
        }

        if (!nextStateName.empty()) {
            currentState->onExit(w, e);
            currentState->onTransition(nextStateName, w, e);

            auto it = states.find(nextStateName);
            auto nextState = it->second.get();
            nextState->onEntry(w, e);
            currentState = nextState;
            nextStateName = "";
        }
    }

    void FSM::addState(std::unique_ptr<BaseFSMState> && state)
    {
        states[state->getName()] = std::move(state);
    }

    void FSM::start(const std::string & name, ecs::World* w, EngineMain* e)
    {
        auto it = states.find(name);        
        auto nextState = it->second.get();
        nextState->onEntry(w, e);
        currentState = nextState;
        nextStateName = "";
    }
}
