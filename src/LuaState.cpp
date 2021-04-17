//
// Created by shane on 13/03/2021.
//

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
}

#include "LuaState.h"
#if 0
#pragma warning( push )
#pragma warning(disable:4505)
#include <LuaBridge/LuaBridge.h>
#pragma warning( pop )
#endif
#include "Vfs.h"
#include "Log.h"

namespace RxEngine
{
    LuaState::LuaState()
    {
        L = luaL_newstate();
        luaopen_base(L);
        luaopen_coroutine(L);
        luaopen_table(L);
        luaopen_math(L);
        luaopen_string(L);
        //auto x = luabridge::getGlobal(L, "fred");
    }

    void LuaState::loadFile(const std::filesystem::path & file)
    {
        auto path = file;
        if (!path.has_extension()) {
            path.replace_extension(".lua");
        }

        const std::string script = RxAssets::vfs()->getStringFile(path);
        int r = luaL_loadbuffer(L, script.c_str(), script.length(), path.generic_string().c_str());

        if (r == 0) {
            if (lua_pcall(L, 0, LUA_MULTRET, 0) != 0) {
                const auto error_string = lua_tostring(L, -1);
                throw std::runtime_error(error_string ? error_string : "Unknown lua error");
            }
        } else {
            spdlog::error("Error in {} - {}", file.generic_string().c_str(), lua_tostring(L, -1));
            throw std::runtime_error(lua_tostring(L, -1));
        }
    }
}
