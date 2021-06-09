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

    bool FSM::setNextState(const std::string & newState)
    {
        if (!states.contains(newState)) {
            return false;
        }
        nextStateName = newState;
        return true;
    }

    std::string FSM::getCurrentState() const
    {
        return currentState->getName();
    }
}
