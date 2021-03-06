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

#include "RxECS.h"
#include "sol/table.hpp"

namespace sol
{
    class state;
}

namespace RxEngine
{
    class EngineMain;

    class Module : public ecs::ModuleBase
    {
    protected:
        EngineMain * engine_;

    public:
        Module(ecs::World * world, EngineMain * engine, ecs::entity_t moduleId)
            : ecs::ModuleBase(world, moduleId)
            , engine_(engine)
        {           
        }

        virtual void startup() {}
        virtual void loadData(sol::table table) {}
        virtual void shutdown() {}

        virtual void registerRuntime(sol::state & lua) {}

        //virtual void processStartupData(sol::state* lua, RxCore::Device* device) {};
    };
}
