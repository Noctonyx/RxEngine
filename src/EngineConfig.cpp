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
        //sol::state lua;

        lua->open_libraries(sol::lib::base, sol::lib::string, sol::lib::table);

        lua->set("dofile", [this](std::string f)
        {
            loadLuaFile(f);
        });

        lua->set("quitgame", [this]()
            {
                shouldQuit = true;
            });

        //auto w = sol::make_light<flecs::world>(*world_);
        //auto e = sol::make_light<EngineMain>(*this);
        //lua["world"] = w.void_value();
        //lua["engine"].set(e);// = e.void_value();
#if 0
        lua.set(script_key_world, world_.get());
        lua.set(script_key_engine, this);

        lua.set_function("createMaterialPipeline", [](flecs::world * l, std::string name)
        {
            auto e = l->entity(name.c_str()).add<RxEngine::Render::MaterialPipeline>();
            //return e;
            return MaterialPipelineLua{e};
        });

        auto x = lua.new_usertype<MaterialPipelineLua>("MaterialPipelineLua");
        //x["test"] = &MaterialPipelineLua::x;
        x.set("x", sol::readonly(&MaterialPipelineLua::x));
        x.set("lineWidth",
              sol::property(&MaterialPipelineLua::getLineWidth,
                            &MaterialPipelineLua::setLineWidth));
#endif
        //x.set_function()
#if 0
        luabridge::getGlobalNamespace(lua_->L)
            .beginNamespace("ind")
            .beginClass<MaterialPipelineLua>("MaterialPipeline")
            //.addConstructor<void (*)(const char *)>()
            .addProperty("x", &MaterialPipelineLua::x)
            .endClass()          
            .addFunction("getMaterialPipeline", [this](const char * name)-> std::optional<MaterialPipelineLua>
            {
                auto e = world_->lookup(name);
                if(e.is_valid()) {
                    return MaterialPipelineLua(e);
                }
                return std::nullopt;
            })
            .addFunction("createOpaquePipeline", [this]()
            {
                auto e = world_->entity();
                e.add<Render::OpaquePipeline>();

            })

            .endNamespace();
#endif
    } 
}
