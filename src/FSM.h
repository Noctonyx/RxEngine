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

#pragma once
#include <functional>
#include "RxECS.h"

namespace RxEngine
{
    class FSM;
    class EngineMain;

    struct BaseFSMState
    {
        //std::string name;
    public:
        virtual ~BaseFSMState() = default;

        virtual void onEntry(ecs::World * world, EngineMain * engine)
        {}

        virtual void onExit(ecs::World * world, EngineMain * engine)
        {}

        virtual void step(FSM * fsm, ecs::World * world, EngineMain * engine)
        {}

#if 0
        virtual void onTransition(std::string_view target,
                                  ecs::World * world,
                                  EngineMain * engine) {}
#endif
        virtual std::string getName() const = 0;
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
    class FSM
    {
    private:
        BaseFSMState * currentState = nullptr;
        std::string nextStateName;

        std::unordered_map<std::string, std::unique_ptr<BaseFSMState>> states;
    public:
        void step(ecs::World *, EngineMain *);
        void addState(std::unique_ptr<BaseFSMState> && state);

        bool setNextState(const std::string & newState);
        [[nodiscard]] std::string getCurrentState() const;

        void start(const std::string & name, ecs::World * w, EngineMain * e);
    };
}
