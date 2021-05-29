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
#add_subdirectory("extern/bitsery")

set(tinygltf_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/tinygltf)

add_subdirectory(extern/RmlUi)

option(LUA_BUILD_COMPILER OFF)
add_subdirectory(extern/lua)
add_subdirectory(extern/lunasvg)
add_subdirectory(extern/sol2)
add_subdirectory(extern/freetype)
add_subdirectory(extern/dxmath)

add_subdirectory("RxUtil")
add_subdirectory("RxCore")
add_subdirectory("RxAssets")
add_subdirectory("RxECS")
add_subdirectory("ImportGLTF")

add_library(RxEngine STATIC 
        src/Modules/ImGui/ImGuiRender.cpp
        src/Modules/ImGui/ImGuiRender.hpp
        src/EngineMain.hpp
        src/EngineMain.cpp
        src/Window.cpp
        src/Window.hpp
        src/Modules/Renderer/Renderer.hpp
        src/Modules/Renderer/Renderer.cpp
        src/Geometry/Camera.hpp
        src/Rendering/MaterialManager.cpp
        src/ini.h
        src/Rendering/MaterialManager.h
        src/Rendering/Defines.h
        src/Modules/Render.h
        src/Modules/Render.cpp
        src/Modules/Transforms/Transforms.h
        src/EngineConfig.h
        src/EngineConfig.cpp
        src/EngineUI.cpp 
        src/Modules/Stats/Stats.h
        src/Modules/Stats/Stats.cpp
        src/Modules/Module.h 
        src/Modules/Transforms/Transforms.cpp 
        src/Modules/RmlUI/RmlUI.h 
        src/Modules/RmlUI/RmlUI.cpp
        src/Modules/SceneCamera/SceneCamera.h
        src/Modules/SceneCamera/SceneCamera.cpp
        src/Modules/StaticMesh/StaticMesh.h
        src/Modules/StaticMesh/StaticMesh.cpp
        src/Modules/Prototypes/Prototypes.h
        src/Modules/Prototypes/Prototypes.cpp
        src/Modules/WorldObject/WorldObject.h
        src/Modules/WorldObject/WorldObject.cpp 
        src/Modules/Materials/Materials.h 
        src/Modules/Materials/Materials.cpp
        src/Modules/RTSCamera/RTSCamera.h 
        src/Modules/RTSCamera/RTSCamera.cpp 
        src/Modules/Lighting/Lighting.h 
        src/Modules/Lighting/Lighting.cpp 
        src/Modules/DynamicMesh/DynamicMesh.h
        src/Modules/DynamicMesh/DynamicMesh.cpp
        src/Modules/Mesh/Mesh.h
        src/Modules/Mesh/Mesh.cpp  "src/FSM.h" "src/FSM.cpp")

target_compile_options(RxEngine PUBLIC -DGLFW_INCLUDE_VULKAN -DVK_USE_PLATFORM_WIN32_KHR -DGLFW_EXPOSE_NATIVE_WIN32 -DNOMINMAX)

if (MSVC)
    target_compile_options(RxEngine PUBLIC /W4 /WX /wd4100 -D_CRT_SECURE_NO_WARNINGS -D_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING -DWIN32_LEAN_AND_MEAN)
    target_compile_options(RxEngine PUBLIC -DSOL_ALL_SAFETIES_ON=1)
    target_compile_options(RxEngine PUBLIC -DECS_JOBS)
    set_source_files_properties(${imgui_SOURCE_DIR}/imgui_widgets.cpp COMPILE_FLAGS /wd4189)
    set_source_files_properties(${imgui_SOURCE_DIR}/imgui_widgets.cpp COMPILE_FLAGS /wd5054)
    set_source_files_properties(${imgui_SOURCE_DIR}/imgui.cpp COMPILE_FLAGS /wd5054)
    set_source_files_properties(${imgui_SOURCE_DIR}/imgui_tables.cpp COMPILE_FLAGS /wd5054)
    set_source_files_properties(${imgui_SOURCE_DIR}/imgui_draw.cpp COMPILE_FLAGS /wd5054)
    set_source_files_properties(${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp COMPILE_FLAGS /wd5054)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set_source_files_properties(${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp COMPILE_FLAGS -Wno-unused-function)
    else()
        target_compile_options(RxEngine PUBLIC /GL)
    endif()
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
target_link_libraries(RxEngine PUBLIC RmlSVG)
#target_link_libraries(RxEngine PUBLIC glfw)
target_link_libraries(RxEngine PUBLIC DirectXMath)

target_include_directories(RxEngine PUBLIC ${imgui_SOURCE_DIR})
target_sources(RxEngine PRIVATE ${imgui_SOURCE_DIR}/imgui_draw.cpp ${imgui_SOURCE_DIR}/imgui_widgets ${imgui_SOURCE_DIR}/imgui_demo.cpp ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp )
target_compile_options(RxEngine PUBLIC -DIMGUI_ENABLE_FREETYPE)

set_target_properties(RxEngine PROPERTIES CXX_STANDARD 20)