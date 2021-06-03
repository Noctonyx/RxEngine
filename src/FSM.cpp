#include "FSM.h"

namespace RxEngine
{
    void FSM::step(ecs::World * w, EngineMain * e)
    {
        if (currentState) {
            currentState->step(this, w, e);
        }

        if (!nextStateName.empty()) {
            if (currentState) {
                currentState->onExit(w, e);
            }
            //currentState->onTransition(nextStateName, w, e);

            const auto it = states.find(nextStateName);
            auto next_state = it->second.get();
            next_state->onEntry(w, e);
            currentState = next_state;
            nextStateName = "";
        }
    }

    void FSM::addState(std::unique_ptr<BaseFSMState> && state)
    {
        states[state->getName()] = std::move(state);
    }

    void FSM::start(const std::string & name, ecs::World * w, EngineMain * e)
    {
        auto it = states.find(name);
        auto nextState = it->second.get();
        nextState->onEntry(w, e);
        currentState = nextState;
        nextStateName = "";
    }
}
