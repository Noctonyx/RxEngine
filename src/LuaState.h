//
// Created by shane on 13/03/2021.
//

#ifndef RX_LUASTATE_H
#define RX_LUASTATE_H

#include <filesystem>
extern "C" {
#include <lua.h>
}

namespace RxEngine
{
    class LuaState
    {
    public:
        LuaState();
        void loadFile(const std::filesystem::path & file);
    public:
        lua_State * L;
    };
}
#endif //RX_LUASTATE_H
