#include "EngineMain.hpp"
#include "EngineConfig.h"
#include "Modules/Render.h"
#include <sol/sol.hpp>
#include "AssetException.h"
#include "SerialisationData.h"
#include "Loader.h"
#include "RxECS.h"
#include "Modules/StaticMesh/StaticMesh.h"


static const auto & script_key_world = "GlobalResource.World";
static const auto & script_key_engine = "GlobalResource.Engine";
//static const auto& script_key = "GlobalResource.MySpecialIdentifier123";

template <typename Handler>
bool sol_lua_check(sol::types<ecs::World *>,
                   lua_State * L,
                   int /*index*/,
                   Handler && handler,
                   sol::stack::record & tracking)
{
    // not actually taking anything off the stack
    tracking.use(0);
    // get the field from global storage
    sol::stack::get_field<true>(L, script_key_world);
    // verify type
    sol::type t = static_cast<sol::type>(lua_type(L, -1));
    lua_pop(L, 1);
    if (t != sol::type::lightuserdata) {
        handler(L, 0, sol::type::lightuserdata, t,
                "global resource is not present as a light userdata");
        return false;
    }
    return true;
}

ecs::World * sol_lua_get(sol::types<ecs::World *>,
                         lua_State * L,
                         int /*index*/,
                         sol::stack::record & tracking)
{
    // retrieve the (light) userdata for this type

    // not actually pulling anything off the stack
    tracking.use(0);
    sol::stack::get_field<true>(L, script_key_world);
    auto ls = static_cast<ecs::World *>(lua_touserdata(L, -1));

    // clean up stack value returned by `get_field`
    lua_pop(L, 1);
    return ls;
}

int sol_lua_push(sol::types<ecs::World *>, lua_State * L, ecs::World * ls)
{
    // push light userdata
    return sol::stack::push(L, static_cast<void *>(ls));
}


template <typename Handler>
bool sol_lua_check(sol::types<RxEngine::EngineMain *>,
                   lua_State * L,
                   int /*index*/,
                   Handler && handler,
                   sol::stack::record & tracking)
{
    // not actually taking anything off the stack
    tracking.use(0);
    // get the field from global storage
    sol::stack::get_field<true>(L, script_key_engine);
    // verify type
    sol::type t = static_cast<sol::type>(lua_type(L, -1));
    lua_pop(L, 1);
    if (t != sol::type::lightuserdata) {
        handler(L, 0, sol::type::lightuserdata, t,
                "global resource is not present as a light userdata");
        return false;
    }
    return true;
}

RxEngine::EngineMain * sol_lua_get(sol::types<RxEngine::EngineMain *>,
                                   lua_State * L,
                                   int /*index*/,
                                   sol::stack::record & tracking)
{
    // retrieve the (light) userdata for this type

    // not actually pulling anything off the stack
    tracking.use(0);
    sol::stack::get_field<true>(L, script_key_engine);
    auto engine = static_cast<RxEngine::EngineMain *>(lua_touserdata(L, -1));

    // clean up stack value returned by `get_field`
    lua_pop(L, 1);
    return engine;
}

int sol_lua_push(sol::types<RxEngine::EngineMain *>, lua_State * L, RxEngine::EngineMain * engine)
{
    // push light userdata
    return sol::stack::push(L, static_cast<void *>(engine));
}


namespace RxEngine
{
    void EngineMain::setupLuaEnvironment()
    {
        lua->open_libraries(sol::lib::base, sol::lib::string, sol::lib::table, sol::lib::package);

        lua->set("dofile", [this](std::string f)
        {
            loadLuaFile(f);
        });

        //lua->clear_package_loaders();
        //lua->add_package_loader()
        sol::table package = (*lua)["package"];
        sol::table loaders = package["searchers"];
        sol::function f = loaders[1];

        sol::table t = lua->create_table();
        //t[1] = l;
        t[1] = f;
        t[2] = [](lua_State * L)
        {
            std::string mod = sol::stack::get<std::string>(L, 1);
            std::string path = "/lua/" + mod + ".lua";
            //spdlog::info("Trying to load {0}", path);

            const std::string script = RxAssets::vfs()->getStringFile(path);
            luaL_loadbuffer(L, script.data(), script.size(), path.c_str());
            return 1;
        };

        package.set("searchers", t);

        lua->set("quitgame", [this]()
        {
            shouldQuit = true;
        });
    }
}
