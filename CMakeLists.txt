cmake_minimum_required(VERSION 3.15)

#set(RXENGINE_ASSET_PATH "${PROJECT_SOURCE_DIR}/Assets")
#set(RXENGINE_ASSET_PATH "${RXENGINE_ASSET_PATH}" PARENT_PATH)

option(GLFW_BUILD_DOCS OFF)
option(GLFW_INSTALL OFF)
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)

add_subdirectory("extern/glfw")

option(JSON_BuildTests "Build the unit tests when BUILD_TESTING is enabled." OFF)
add_subdirectory("extern/json")
add_subdirectory("extern/spdlog")

set(imgui_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui)
add_subdirectory("extern/bitsery")

set(tinygltf_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/tinygltf)

add_subdirectory(extern/RmlUi)

option(LUA_BUILD_COMPILER OFF)
add_subdirectory(extern/lua)
add_subdirectory(extern/sol2)
add_subdirectory(extern/freetype)
add_subdirectory(extern/dxmath)

add_subdirectory("RxUtil")
add_subdirectory("RxCore")
add_subdirectory("RxAssets")
add_subdirectory("RxECS")
add_subdirectory("ImportGLTF")

add_library(RxEngine STATIC 
        src/ImGui/ImGuiRender.cpp
        "src/ImGui/ImGuiRender.hpp"
        src/EngineMain.hpp
        src/EngineMain.cpp
        src/Delegates.hpp
        src/Delegates.cpp
        src/Input/Keyboard.cpp
        src/Input/Keyboard.hpp
        src/Input/Mouse.cpp
        src/Input/Mouse.hpp
        src/Window.cpp
        src/Window.hpp
        src/Scene.h
        src/Scene.cpp
        src/Example/TestMesh.cpp
        src/Example/TestMesh.hpp
        src/Rendering/RenderMesh.h
        src/Rendering/Mesh.cpp
        src/Rendering/Mesh.hpp
        src/Rendering/Renderer.hpp
        src/Rendering/Renderer.cpp
        src/Rendering/RenderData.h
        src/Subsystems/SceneCamera.h
        src/Subsystems/SceneCamera.cpp
        
        
        src/FastNoiseLite.h
        src/Subsystems/Lighting.h
        src/Subsystems/Lighting.cpp
        src/Geometry/Camera.hpp
        src/ImGui/ImGuiPipelineLayout.cpp
        src/ImGui/ImGuiPipelineLayout.h
        src/Rendering/ILightingManager.h
        "src/Modules/RmlUI/RmlSystemInterface.cpp"
        "src/Modules/RmlUI/RmlSystemInterface.h"
        "src/Modules/RmlUI/RmlRenderInterface.cpp"
        "src/Modules/RmlUI/RmlRenderInterface.h"
        "src/Modules/RmlUI/RmlFileInterface.cpp"
        "src/Modules/RmlUI/RmlFileInterface.h"
        "src/Modules/RmlUI/UiContext.cpp"
        "src/Modules/RmlUI/UiContext.h"
        
        
        
        
        src/Rendering/MeshBundle.cpp
        src/Rendering/MeshBundle.h
        src/Rendering/RenderCamera.cpp
        src/Rendering/RenderCamera.h
        src/Rendering/MaterialManager.cpp
        src/ini.h
        src/Rendering/MaterialManager.h
        src/Rendering/Defines.h
        src/Rendering/EntityManager.cpp
        src/Rendering/EntityManager.h 
        src/LuaState.cpp 
        src/LuaState.h 
        src/Modules/Render.h
        src/Modules/Render.cpp
        "src/Modules/Transforms/Transforms.h"
        src/EngineConfig.h
        src/EngineConfig.cpp
        src/EngineUI.cpp 
        src/Modules/Stats/Stats.h
        src/Modules/Stats/Stats.cpp
        src/Modules/Module.h 
        src/Modules/Transforms/Transforms.cpp 
 "src/Modules/RmlUI/RmlUI.h" "src/Modules/RmlUI/RmlUI.cpp" "src/Modules/SceneCamera/SceneCamera.h" "src/Modules/SceneCamera/SceneCamera.cpp")

target_compile_options(RxEngine PUBLIC -DGLFW_INCLUDE_VULKAN -DVK_USE_PLATFORM_WIN32_KHR -DGLFW_EXPOSE_NATIVE_WIN32 -DNOMINMAX)

if (MSVC)
    target_compile_options(RxEngine PUBLIC /W4 /WX /wd4100 -D_CRT_SECURE_NO_WARNINGS -D_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING -DWIN32_LEAN_AND_MEAN)
    target_compile_options(RxEngine PUBLIC -DSOL_ALL_SAFETIES_ON=1)
    set_source_files_properties(${imgui_SOURCE_DIR}/imgui_widgets.cpp COMPILE_FLAGS /wd4189)
    set_source_files_properties(${imgui_SOURCE_DIR}/imgui_widgets.cpp COMPILE_FLAGS /wd5054)
    set_source_files_properties(${imgui_SOURCE_DIR}/imgui.cpp COMPILE_FLAGS /wd5054)
    set_source_files_properties(${imgui_SOURCE_DIR}/imgui_tables.cpp COMPILE_FLAGS /wd5054)
    set_source_files_properties(${imgui_SOURCE_DIR}/imgui_draw.cpp COMPILE_FLAGS /wd5054)
    set_source_files_properties(${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp COMPILE_FLAGS /wd5054)
    #set_source_files_properties(${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp COMPILE_FLAGS -Wno-unused-function)
endif ()

target_include_directories(RxEngine PUBLIC "src")

target_include_directories(RxEngine INTERFACE "include")

target_link_libraries(RxEngine PUBLIC RxCore)
target_link_libraries(RxEngine PUBLIC glfw)
target_link_libraries(RxEngine PUBLIC RxECS)
target_link_libraries(RxEngine PUBLIC RxAssets)
target_link_libraries(RxEngine PUBLIC lua_static)
target_link_libraries(RxEngine PUBLIC sol2)
target_link_libraries(RxEngine PUBLIC RmlCore)
target_link_libraries(RxEngine PUBLIC RmlDebugger)
target_link_libraries(RxEngine PUBLIC freetype)
target_link_libraries(RxEngine PUBLIC RmlLua)
#target_link_libraries(RxEngine PUBLIC glfw)
target_link_libraries(RxEngine PUBLIC DirectXMath)

target_include_directories(RxEngine PUBLIC ${imgui_SOURCE_DIR})
target_sources(RxEngine PRIVATE ${imgui_SOURCE_DIR}/imgui_draw.cpp ${imgui_SOURCE_DIR}/imgui_widgets ${imgui_SOURCE_DIR}/imgui_demo.cpp ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp )
target_compile_options(RxEngine PUBLIC -DIMGUI_ENABLE_FREETYPE)

set_target_properties(RxEngine PROPERTIES CXX_STANDARD 20)

