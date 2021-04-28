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


    void loadShaderData(ecs::World * world, RxCore::Device * device, sol::table & shaders)
    {
        //sol::table shaders = lua["data"]["shaders"];

        for (auto & [key, value]: shaders) {
            auto name = key.as<std::string>();
            sol::table data = value;
            auto stage = data.get<std::string>("stage");
            auto spv = data.get<std::string>("shader");

            RxAssets::ShaderData sd;
            RxAssets::Loader::loadShader(sd, spv);

            auto sh = device->createShader(sd.bytes);

            if (stage == "vert") {
                world->newEntity(name.c_str()).set<VertexShader>({
                    .shader = sh, .shaderAssetName = spv
                });
            } else {
                world->newEntity(name.c_str()).set<FragmentShader>({
                    .shader = sh, .shaderAssetName = spv
                });
            }
        }
    }

    RxAssets::MaterialPipelineBlendFactor getBlendFactor(const std::string & value)
    {
        if (value == "zero") {
            return RxAssets::MaterialPipelineBlendFactor::eZero;
        }
        if (value == "one") {
            return RxAssets::MaterialPipelineBlendFactor::eOne;
        }
        if (value == "src-color") {
            return RxAssets::MaterialPipelineBlendFactor::eSrcColor;
        }
        if (value == "1-src-color") {
            return RxAssets::MaterialPipelineBlendFactor::eOneMinusSrcColor;
        }
        if (value == "dest-color") {
            return RxAssets::MaterialPipelineBlendFactor::eDstColor;
        }
        if (value == "src-alpha") {
            return RxAssets::MaterialPipelineBlendFactor::eSrcAlpha;
        }
        if (value == "1-src-alpha") {
            return RxAssets::MaterialPipelineBlendFactor::eOneMinusSrcAlpha;
        }
        if (value == "dest-alpha") {
            return RxAssets::MaterialPipelineBlendFactor::eDstAlpha;
        }
        throw std::runtime_error(
            R"(Invalid value for blendFactor - valid values: "zero", "one", "src-color", "1-src-color", "dest-color", "src-alpha", "1-src-alpha", "dest-alpha")");
    }

    RxAssets::MaterialPipelineBlendOp getBlendOp(const std::string & value)
    {
        if (value == "add") {
            return RxAssets::MaterialPipelineBlendOp::eAdd;
        }
        if (value == "subtract") {
            return RxAssets::MaterialPipelineBlendOp::eSubtract;
        }
        if (value == "reverse-subtract") {
            return RxAssets::MaterialPipelineBlendOp::eReverseSubtract;
        }
        if (value == "min") {
            return RxAssets::MaterialPipelineBlendOp::eMin;
        }
        if (value == "max") {
            return RxAssets::MaterialPipelineBlendOp::eMax;
        }
        throw std::runtime_error(
            R"(Invalid value for blendOp - valid values: "add", "subtract", "reverse-subtract", "min", "max")");
    }

    void populateMaterialPipelineDetails(sol::table & details,
                                         MaterialPipelineDetails & mpd)
    {
        mpd.depthTestEnable = details.get_or("depthTestEnable", false);
        mpd.depthWriteEnable = details.get_or("depthWriteEnable", false);
        mpd.stencilTest = details.get_or("stencilTest", false);
        mpd.depthClamp = details.get_or("depthClamp", false);

        mpd.lineWidth = details.get_or("lineWidth", 1.0f);
        mpd.minDepth = details.get_or("minDepth", 0.0f);
        mpd.maxDepth = details.get_or("maxDepth", 1.0f);

        std::string stage = details.get_or("renderStage", std::string{"opaque"});
        if (stage == "opaque") {
            mpd.stage = RxAssets::PipelineRenderStage::Opaque;
        } else if (stage == "shadow") {
            mpd.stage = RxAssets::PipelineRenderStage::Shadow;
        } else if (stage == "transparent") {
            mpd.stage = RxAssets::PipelineRenderStage::Transparent;
        } else if (stage == "ui") {
            mpd.stage = RxAssets::PipelineRenderStage::UI;
        } else {
            throw std::runtime_error(
                R"(Invalid value for renderStage - valid values: "opaque", "shadow", "transparent", "ui")");
        }

        std::string fillMode = details.get_or("fillMode", std::string{"fill"});
        if (fillMode == "fill") {
            mpd.fillMode = RxAssets::MaterialPipelineFillMode::eFill;
        } else if (fillMode == "line") {
            mpd.fillMode = RxAssets::MaterialPipelineFillMode::eLine;
        } else if (fillMode == "point") {
            mpd.fillMode = RxAssets::MaterialPipelineFillMode::ePoint;
        } else {
            throw std::runtime_error(
                R"(Invalid value for fillMode - valid values: "fill", "line", "point")");
        }

        std::string cullMode = details.get_or("cullMode", std::string{"back"});

        if (cullMode == "back") {
            mpd.cullMode = RxAssets::MaterialPipelineCullMode::eBack;
        } else if (cullMode == "front") {
            mpd.cullMode = RxAssets::MaterialPipelineCullMode::eFront;
        } else if (cullMode == "both") {
            mpd.cullMode = RxAssets::MaterialPipelineCullMode::eFrontAndBack;
        } else if (cullMode == "none") {
            mpd.cullMode = RxAssets::MaterialPipelineCullMode::eNone;
        } else {
            throw std::runtime_error(
                R"(Invalid value for fillMode - valid values: "back", "front", "both", "none")");
        }

        std::string frontFace = details.get_or("frontFace", std::string{"counter-clockwise"});
        if (frontFace == "counter-clockwise") {
            mpd.frontFace = RxAssets::MaterialPipelineFrontFace::eCounterClockwise;
        } else if (frontFace == "clockwise") {
            mpd.frontFace = RxAssets::MaterialPipelineFrontFace::eClockwise;
        } else {
            throw std::runtime_error(
                R"(Invalid value for frontFace - valid values: "counter-clockwise", "clockwise")");
        }

        std::string depthCompare = details.get_or("depthCompare", std::string{"less-equal"});
        if (depthCompare == "never") {
            mpd.depthCompareOp = RxAssets::MaterialPipelineDepthCompareOp::eNever;
        } else if (depthCompare == "less") {
            mpd.depthCompareOp = RxAssets::MaterialPipelineDepthCompareOp::eLess;
        } else if (depthCompare == "equal") {
            mpd.depthCompareOp = RxAssets::MaterialPipelineDepthCompareOp::eEqual;
        } else if (depthCompare == "less-equal") {
            mpd.depthCompareOp = RxAssets::MaterialPipelineDepthCompareOp::eLessOrEqual;
        } else if (depthCompare == "greater") {
            mpd.depthCompareOp = RxAssets::MaterialPipelineDepthCompareOp::eGreater;
        } else if (depthCompare == "not-equal") {
            mpd.depthCompareOp = RxAssets::MaterialPipelineDepthCompareOp::eNotEqual;
        } else if (depthCompare == "greater-equal") {
            mpd.depthCompareOp = RxAssets::MaterialPipelineDepthCompareOp::eGreaterOrEqual;
        } else if (depthCompare == "always") {
            mpd.depthCompareOp = RxAssets::MaterialPipelineDepthCompareOp::eAlways;
        } else {
            throw std::runtime_error(
                R"(Invalid value for depthCompare - valid values: "never", "less", "equal", "less-equal", "greater", "not-equal", "greater-equal", "always")");
        }

        sol::table vertices = details["vertices"];
        for (auto & [key, value]: vertices) {
            uint32_t count;
            uint32_t offset;
            sol::table v = value;

            count = v.get<uint32_t>("count");
            offset = v.get<uint32_t>("offset");

            auto & ip = mpd.inputs.emplace_back();
            ip.offset = offset;
            ip.count = count;

            if (v.get<std::string>("type") == std::string{"float"}) {
                ip.inputType = RxAssets::MaterialPipelineInputType::eFloat;
            } else if (v.get<std::string>("type") == std::string{"byte"}) {
                ip.inputType = RxAssets::MaterialPipelineInputType::eByte;
            } else {
                throw std::runtime_error(
                    R"(Invalid value for input type - valid values: "byte", "float")");
            }
        }

        sol::table blends = details["blends"];
        for (auto & [blendKey, blendValue]: blends) {
            sol::table blendData = blendValue;

            auto & blend = mpd.blends.emplace_back();
            //b.enable = false;
            blend.sourceFactor = RxAssets::MaterialPipelineBlendFactor::eSrcAlpha;
            blend.destFactor = RxAssets::MaterialPipelineBlendFactor::eOneMinusSrcAlpha;
            blend.colorBlendOp = RxAssets::MaterialPipelineBlendOp::eAdd;
            blend.sourceAlphaFactor = RxAssets::MaterialPipelineBlendFactor::eOneMinusSrcAlpha;
            blend.destAlphaFactor = RxAssets::MaterialPipelineBlendFactor::eZero;
            blend.alphaBlendOp = RxAssets::MaterialPipelineBlendOp::eAdd;

            blend.enable = blendData.get_or("enable", false);

            if (sol::optional<std::string> f = blendData["sourceFactor"];
                f != sol::nullopt) {
                blend.sourceFactor = getBlendFactor(f.value());
            }
            if (sol::optional<std::string> f = blendData["destFactor"];
                f != sol::nullopt) {
                blend.destFactor = getBlendFactor(f.value());
            }
            if (sol::optional<std::string> f = blendData["sourceAlphaFactor"];
                f != sol::nullopt) {
                blend.sourceAlphaFactor = getBlendFactor(f.value());
            }
            if (sol::optional<std::string> f = blendData["destAlphaFactor"];
                f != sol::nullopt) {
                blend.destAlphaFactor = getBlendFactor(f.value());
            }
            if (sol::optional<std::string> f = blendData["colorOp"];
                f != sol::nullopt) {
                blend.colorBlendOp = getBlendOp(f.value());
            }
            if (sol::optional<std::string> f = blendData["alphaOp"];
                f != sol::nullopt) {
                blend.alphaBlendOp = getBlendOp(f.value());
            }
        }
    }

    vk::ShaderStageFlags getStageFlags(const std::string & stage)
    {
        if (stage == "both") {
            return vk::ShaderStageFlagBits::eFragment |
                vk::ShaderStageFlagBits::eVertex;
        } else if (stage == "vert") {
            return vk::ShaderStageFlagBits::eVertex;
        } else if (stage == "frag") {
            return vk::ShaderStageFlagBits::eFragment;
        } else {
            throw std::runtime_error(
                R"(Invalid value for stage - valid values: "both", "frag", "vert")");
        }

        //return {};
    }

    void loadLayout(ecs::World * world,
                    RxCore::Device * device,
                    const std::string & name,
                    sol::table & layout)
    {
        //Render::PipelineLayoutDetails pld;

        vk::PipelineLayoutCreateInfo plci{};
        std::vector<vk::DescriptorSetLayout> dsls{};
        std::vector<vk::PushConstantRange> pcr{};

        PipelineLayout pll;

        sol::table dsLayouts = layout["ds_layouts"];
        for (auto & [dsLayoutKey, dsLayoutValue]: dsLayouts) {
            sol::table dsLayoutData = dsLayoutValue;

            std::vector<vk::DescriptorSetLayoutBinding> binding = {};
            std::vector<vk::DescriptorBindingFlags> binding_flags = {};

            sol::table bindings = dsLayoutData["bindings"];
            for (auto & [bindingKey, bindingValue]: bindings) {
                sol::table bindingData = bindingValue;
                auto & b = binding.emplace_back();
                auto & bf = binding_flags.emplace_back();


                b.binding = bindingData.get<uint32_t>("binding");
                b.descriptorCount = bindingData.get_or<uint32_t>("count", 1);

                std::string stage = bindingData.get_or("stage", std::string{"both"});
                std::string type = bindingData.get_or("type", std::string{"combined-sampler"});
                b.stageFlags = getStageFlags(stage);

                if (type == "combined-sampler") {
                    b.descriptorType = vk::DescriptorType::eCombinedImageSampler;
                } else if (type == "storage-buffer") {
                    b.descriptorType = vk::DescriptorType::eStorageBuffer;
                } else if (type == "uniform-buffer") {
                    b.descriptorType = vk::DescriptorType::eUniformBuffer;
                } else if (type == "uniform-buffer-dynamic") {
                    b.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
                } else if (type == "storage-buffer-dynamic") {
                    b.descriptorType = vk::DescriptorType::eStorageBufferDynamic;
                } else {
                    throw std::runtime_error(
                        R"(Invalid value for stage - valid values: "combined-sampler", "storage-buffer", "uniform-buffer", "storage-buffer-dynamic", "uniform-buffer-dynamic")");
                }

                if (bindingData.get_or("variable", false)) {
                    bf |= vk::DescriptorBindingFlagBits::eVariableDescriptorCount;
                }
                if (bindingData.get_or("partially_bound", false)) {
                    bf |= vk::DescriptorBindingFlagBits::ePartiallyBound;
                }
                if (bindingData.get_or("update_after", false)) {
                    bf |= vk::DescriptorBindingFlagBits::eUpdateAfterBind;
                }
            }

            vk::DescriptorSetLayoutBindingFlagsCreateInfo dslbfc{};
            dslbfc.setBindingFlags(binding_flags);

            vk::DescriptorSetLayoutCreateInfo dslci{};
            dslci.setFlags(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool);

            dslci.setPNext(&dslbfc);
            dslci.setBindings(binding);

            auto dsl = device->createDescriptorSetLayout(dslci);

            pll.dsls.push_back(dsl);
            dsls.push_back(dsl);
        }

        sol::table pushConstants = layout["push_constants"];
        for (auto & [pcKey, pcValue]: pushConstants) {
            sol::table pcData = pcValue;

            auto & p = pcr.emplace_back();
            std::string stage = pcData.get_or("stage", std::string{"both"});
            p.stageFlags = getStageFlags(stage);
            p.offset = pcData.get<uint32_t>("offset");
            p.size = pcData.get<uint32_t>("size");
        }

        plci.setSetLayouts(dsls)
            .setPushConstantRanges(pcr);

        auto l = device->createPipelineLayout(plci);

        pll.layout = l;
        world->newEntity(name.c_str())
             .set<PipelineLayout>(pll);
    }

    void loadLayouts(ecs::World * world, RxCore::Device * device, sol::table & layouts)
    {
        for (auto & [key, value]: layouts) {
            auto layoutName = key.as<std::string>();

            sol::table details = value;

            loadLayout(world, device, layoutName, details);
        }
    }

    void loadPipeline(ecs::World * world,
                      RxCore::Device * device,
                      const std::string & name,
                      sol::table & pipeline)
    {
        const std::string vs_name = pipeline["vertexShader"];
        const std::string fs_name = pipeline["fragmentShader"];
        const std::string layout_name = pipeline["layout"];

        MaterialPipelineDetails mpd;

        populateMaterialPipelineDetails(pipeline, mpd);

        auto vse = world->lookup(vs_name.c_str());
        if (!vse.isAlive() || !vse.has<VertexShader>()) {
            spdlog::critical("Missing shader for pipeline {}", name.c_str());
            throw RxAssets::AssetException("missing shader:", vs_name);
        }

        auto fse = world->lookup(fs_name.c_str());
        if (!fse.isAlive() || !fse.has<FragmentShader>()) {
            spdlog::critical("Missing shader for pipeline {}", name.c_str());
            throw RxAssets::AssetException("missing shader:", fs_name);
        }

        auto lay = world->lookup(layout_name.c_str());
        if (!lay.isAlive() || !lay.has<PipelineLayout>()) {
            spdlog::critical("Missing layout for pipeline {}", name.c_str());
            throw RxAssets::AssetException("missing shader:", layout_name);
        }

        world->newEntity(name.c_str())
             .set<MaterialPipelineDetails>({mpd})
             .set<UsesVertexShader>({{vse.id}})
             .set<UsesFragmentShader>({{fse.id}})
             .set<UsesLayout>({{lay.id}});
    }

    void loadPipelines(ecs::World * world, RxCore::Device * device, sol::table & pipelines)
    {
        for (auto & [key, value]: pipelines) {
            auto pipelineName = key.as<std::string>();

            sol::table details = value;

            loadPipeline(world, device, pipelineName, details);
        }
    }

    void getSamplerDetails(RxAssets::SamplerData & sd, sol::table & sampler)
    {
        std::string minFilter = sampler.get_or("minFilter", std::string{"nearest"});
        if (minFilter == "nearest") {
            sd.minFilter = VK_FILTER_NEAREST;
        } else if (minFilter == "linear") {
            sd.minFilter = VK_FILTER_LINEAR;
        } else {
            throw std::runtime_error(
                R"(Invalid value for minFilter - valid values: "nearest", "linear")");
        }

        std::string magFilter = sampler.get_or("magFilter", std::string{"nearest"});
        if (magFilter == "nearest") {
            sd.magFilter = VK_FILTER_NEAREST;
        } else if (magFilter == "linear") {
            sd.magFilter = VK_FILTER_LINEAR;
        } else {
            throw std::runtime_error(
                R"(Invalid value for magFilter - valid values: "nearest", "linear")");
        }

        std::string mipMapMode = sampler.get_or("mipMapMode", std::string{"nearest"});
        if (mipMapMode == "nearest") {
            sd.mipMapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        } else if (mipMapMode == "linear") {
            sd.mipMapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        } else {
            throw std::runtime_error(
                R"(Invalid value for mipMapMode - valid values: "nearest", "linear")");
        }

        std::string addressU = sampler.get_or("addressU", std::string{"nearest"});
        if (addressU == "repeat") {
            sd.addressU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        } else if (addressU == "mirrored-repeat") {
            sd.addressU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        } else if (addressU == "clamp-edge") {
            sd.addressU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        } else if (addressU == "clamp-border") {
            sd.addressU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        } else {
            throw std::runtime_error(
                R"(Invalid value for addressU - valid values: "repeat", "mirrored-repeat", "clamp-edge", "clamp-border")");
        }

        std::string addressV = sampler.get_or("addressV", std::string{"nearest"});
        if (addressV == "repeat") {
            sd.addressV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        } else if (addressV == "mirrored-repeat") {
            sd.addressV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        } else if (addressV == "clamp-edge") {
            sd.addressV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        } else if (addressV == "clamp-border") {
            sd.addressV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        } else {
            throw std::runtime_error(
                R"(Invalid value for addressV - valid values: "repeat", "mirrored-repeat", "clamp-edge", "clamp-border")");
        }

        std::string addressW = sampler.get_or("addressW", std::string{"nearest"});
        if (addressW == "nearest") {
            sd.addressW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        } else if (addressW == "mirrored-repeat") {
            sd.addressW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        } else if (addressW == "clamp-edge") {
            sd.addressW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        } else if (addressW == "clamp-border") {
            sd.addressW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        } else {
            throw std::runtime_error(
                R"(Invalid value for addressW - valid values: "repeat", "mirrored-repeat", "clamp-edge", "clamp-border")");
        }

        sd.mipLodBias = sampler.get_or("mipLodBias", 0.f);
        sd.anisotropy = sampler.get_or("anisotropy", false);
        sd.maxAnisotropy = sampler.get_or("maxAnisotropy", 0.f);
        sd.minLod = sampler.get_or("minLod", 0.f);
        sd.maxLod = sampler.get_or("maxLod", VK_LOD_CLAMP_NONE);

        std::string borderColor = sampler.get_or("borderColor",
                                                 std::string{"float-transparent-black"});
        if (borderColor == "float-transparent-black") {
            sd.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        } else if (borderColor == "int-transparent-black") {
            sd.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
        } else if (borderColor == "float-opaque-black") {
            sd.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        } else if (borderColor == "int-opaque-black") {
            sd.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        } else if (borderColor == "float-opaque-white") {
            sd.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        } else if (borderColor == "int-opaque-white") {
            sd.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
        } else {
            throw std::runtime_error(
                R"(Invalid value for borderColor - valid values are "float-transparent-black", "int-transparent-black", "float-opaque-black", "int-opaque-black", "float-opaque-white", "int-opaque-white" )");
        }
    }

    vk::Sampler createSampler(RxCore::Device * device, RxAssets::SamplerData & sd)
    {
        vk::SamplerCreateInfo sci;

        sci.setMinFilter(static_cast<vk::Filter>(sd.minFilter));
        sci.setMagFilter(static_cast<vk::Filter>(sd.magFilter));
        sci.setMipmapMode(static_cast<vk::SamplerMipmapMode>(sd.mipMapMode));
        sci.setAddressModeU(static_cast<vk::SamplerAddressMode>(sd.addressU));
        sci.setAddressModeV(static_cast<vk::SamplerAddressMode>(sd.addressV));
        sci.setAddressModeW(static_cast<vk::SamplerAddressMode>(sd.addressW));
        sci.setMipLodBias(sd.mipLodBias);
        sci.setAnisotropyEnable(sd.anisotropy);
        sci.setMaxAnisotropy(sd.maxAnisotropy);
        sci.setMinLod(sd.minLod);
        sci.setMaxLod(sd.maxLod);

        return device->createSampler(sci);
    }

    ecs::EntityHandle loadOrGetImage(const std::string & name,
                                     ecs::World * world,
                                     RxCore::Device * device)
    {
        auto e = world->lookup(name.c_str());

        if (e.isAlive() && e.has<MaterialImage>()) {
            return e;
        }

        RxAssets::ImageData id{};
        RxAssets::Loader::loadImage(id, name);

        auto image = device->createImage(
            id.imType == RxAssets::eBC7
                ? vk::Format::eBc7UnormBlock
                : vk::Format::eR8G8B8A8Unorm,
            vk::Extent3D{id.width, id.height, 1},
            static_cast<uint32_t>(id.mipLevels.size()),
            1,
            vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
            vk::ImageType::e2D);

        auto iv =
            image->createImageView(vk::ImageViewType::e2D, vk::ImageAspectFlagBits::eColor);

        for (uint32_t j = 0; j < id.mipLevels.size(); j++) {
            const auto staging_buffer = device->createStagingBuffer(
                id.mipLevels[j].bytes.size(), id.mipLevels[j].bytes.data());

            device->transferBufferToImage(
                staging_buffer,
                image,
                vk::Extent3D(id.mipLevels[j].width, id.mipLevels[j].height, 1),
                vk::ImageLayout::eShaderReadOnlyOptimal,
                1,
                0,
                j);
        }
        MaterialImage mi{};
        mi.image = image;
        mi.imageView = iv;

        return world->newEntity(name.c_str()).set<MaterialImage>(mi);
    }

    void loadTexture(ecs::World * world,
                     RxCore::Device * device,
                     std::string textureName,
                     sol::table details)
    {
        const std::string file_name = details["image"];

        const auto image_entity = loadOrGetImage(file_name, world, device);

        RxAssets::SamplerData sd{};

        sol::table sampler = details["sampler"];
        getSamplerDetails(sd, sampler);

        const auto sampler_handle = createSampler(device, sd);

        world->newEntity(textureName.c_str())
             .set<ecs::InstanceOf>({{image_entity.id}})
             .set<Render::MaterialSampler>({sampler_handle, 9999});
    }

    void loadTextures(ecs::World * world, RxCore::Device * device, sol::table & textures)
    {
        for (auto & [key, value]: textures) {
            const std::string texture_name = key.as<std::string>();
            sol::table details = value;

            loadTexture(world, device, texture_name, details);
        }
    }

    void loadMaterial(ecs::World * world,
                      RxCore::Device * device,
                      const std::string & name,
                      sol::table & material)
    {
        sol::optional<std::string> opaquePipeline = material["opaquePipeline"];
        sol::optional<std::string> shadowPipeline = material["shadowPipeline"];
        sol::optional<std::string> transparentPipeline = material["transparentPipeline"];
        sol::optional<std::string> uiPipeline = material["uiPipeline"];

        Material mi{};

        auto e = world->lookup(name.c_str());

        if (!e.isAlive()) {
            e = world->newEntity(name.c_str());
        }

        if (opaquePipeline.has_value()) {
            auto eop = world->lookup(opaquePipeline.value().c_str());

            auto mpd = eop.get<MaterialPipelineDetails>();
            if (!mpd || mpd->stage != RxAssets::PipelineRenderStage::Opaque) {
                throw RxAssets::AssetException("Not a valid opaquePipeline:", name);
            }
            e.set<HasOpaquePipeline>({{eop.id}});
        }

        if (shadowPipeline.has_value()) {
            auto eop = world->lookup(shadowPipeline.value().c_str());

            auto mpd = eop.get<MaterialPipelineDetails>();
            if (!mpd || mpd->stage != RxAssets::PipelineRenderStage::Shadow) {
                throw RxAssets::AssetException("Not a valid shadowPipeline:", name);
            }
            e.set<HasShadowPipeline>({{eop.id}});
        }

        if (transparentPipeline.has_value()) {
            auto eop = world->lookup(transparentPipeline.value().c_str());

            auto mpd = eop.get<MaterialPipelineDetails>();
            if (!mpd || mpd->stage != RxAssets::PipelineRenderStage::Transparent) {
                throw RxAssets::AssetException("Not a valid transparentPipeline:", name);
            }
            e.set<HasTransparentPipeline>({{eop.id}});
        }

        if (uiPipeline.has_value()) {
            auto eop = world->lookup(uiPipeline.value().c_str());

            auto mpd = eop.get<MaterialPipelineDetails>();
            if (!mpd || mpd->stage != RxAssets::PipelineRenderStage::UI) {
                throw RxAssets::AssetException("Not a valid uiPipeline:", name);
            }
            e.set<HasUiPipeline>({{eop.id}});
        }

        float roughness = material.get_or("roughness", 0.f);
        float metallic = material.get_or("metallic", 0.f);

        mi.roughness = roughness;
        mi.metallic = metallic;

        sol::optional<std::string> colorTexture = material["color_texture"];

        if (colorTexture.has_value()) {
            auto te = world->lookup(colorTexture.value().c_str());
            if (te.isAlive() && te.has<Render::MaterialSampler>()) {
                mi.materialTextures[0] = te.id;
            } else {
                throw RxAssets::AssetException("Not a valid texture:", name);
            }
        }

        std::string a = material.get_or("alpha_mode", std::string{"OPAQUE"});
        if (a == "OPAQUE") {
            mi.alpha = MaterialAlphaMode::Opaque;
        } else {
            mi.alpha = MaterialAlphaMode::Transparent;
        }

        e.set<Material>(mi);
    }

    void loadMaterials(ecs::World * world, RxCore::Device * device, sol::table & materials)
    {
        for (auto & [key, value]: materials) {
            const std::string name = key.as<std::string>();
            sol::table details = value;
            loadMaterial(world, device, name, details);
        }
    }

    void loadMesh(ecs::World * world,
                  RxCore::Device * device,
                  std::string meshName,
                  sol::table details)
    {
        std::string meshFile = details.get_or("mesh", std::string{""});
        auto vertices = details.get<uint32_t>("vertices");
        auto indices = details.get<uint32_t>("indices");

        auto me = world->newEntity(meshName.c_str()).set<MeshObject>({
            .vertexCount = vertices,
            .indexCount = indices,
            .meshFile = meshFile
        });

        sol::table smtab = details["submeshes"];
        sol::table mtab = details["materials"];

        std::vector<ecs::entity_t> mEntities;

        for (auto & [k, v]: mtab) {
            std::string mn = v.as<std::string>();

            auto me = world->lookup(mn.c_str());
            if (me.isAlive() && me.has<Material>()) {
                mEntities.push_back(me.id);
            }
        }

        for (auto & [k, v]: smtab) {
            sol::table subMeshValue = v;

            uint32_t first_index = subMeshValue.get<uint32_t>("first_index");
            uint32_t index_count = subMeshValue.get<uint32_t>("index_count");
            uint32_t material = subMeshValue.get<uint32_t>("material");

            world->newEntity()
                 .set<ecs::InstanceOf>({{me.id}})
                 .set<SubMesh>({first_index, index_count})
                 .set<UsesMaterial>({{mEntities[material]}});
        }
    }

    void loadMeshes(ecs::World * world, RxCore::Device * device, sol::table & meshes)
    {
        for (auto & [key, value]: meshes) {
            const std::string name = key.as<std::string>();
            sol::table details = value;
            loadMesh(world, device, name, details);
        }
    }

    void loadPrototype(ecs::World * world,
                       RxCore::Device * device,
                       std::string prototypeName,
                       sol::table details) { }

    void loadPrototypes(ecs::World * world, RxCore::Device * device, sol::table & prototypes)
    {
        for (auto & [key, value]: prototypes) {
            const std::string name = key.as<std::string>();
            sol::table details = value;
            loadPrototype(world, device, name, details);
        }
    }

    void EngineMain::populateStartupData()
    {
        sol::table data = lua->get<sol::table>("data");

        sol::table shaders = data["shaders"];
        sol::table layouts = data["pipeline_layouts"];
        sol::table textures = data["textures"];
        sol::table pipelines = data["material_pipelines"];
        sol::table materials = data["materials"];
        sol::table meshes = data["meshes"];
        sol::table prototypes = data["prototypes"];

        loadShaderData(world.get(), device_.get(), shaders);
        loadLayouts(world.get(), device_.get(), layouts);
        loadPipelines(world.get(), device_.get(), pipelines);
        loadTextures(world.get(), device_.get(), textures);
        loadMaterials(world.get(), device_.get(), materials);
        loadMeshes(world.get(), device_.get(), meshes);
        loadPrototypes(world.get(), device_.get(), meshes);
    }
}
