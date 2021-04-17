cmake_minimum_required(VERSION 3.15)

add_library(RxEngine STATIC 
        src/Subsystems/Stats.hpp
        src/Subsystems/Stats.cpp
        src/ImGui/ImGuiRender.cpp
        src/ImGui/ImGuiRender.hpp
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
        src/Subsystems/Input.h
        src/Subsystems/Input.cpp
        src/FastNoiseLite.h
        src/Subsystems/Lighting.h
        src/Subsystems/Lighting.cpp
        src/Geometry/Camera.hpp
        src/ImGui/ImGuiPipelineLayout.cpp
        src/ImGui/ImGuiPipelineLayout.h
        src/Rendering/ILightingManager.h
        src/UI/RmlSystemInterface.cpp
        src/UI/RmlSystemInterface.h
        src/UI/RmlRenderInterface.cpp
        src/UI/RmlRenderInterface.h
        src/UI/RmlFileInterface.cpp
        src/UI/RmlFileInterface.h
        src/Subsystems/UiContext.cpp
        src/Subsystems/UiContext.h
        src/Subsystems/IInputHandler.h
        src/Subsystems/BatchManager.cpp
        src/Subsystems/BatchManager.h
        src/Rendering/BatchData.h
        src/Rendering/MeshBundle.cpp
        src/Rendering/MeshBundle.h
        src/Rendering/RenderCamera.cpp
        src/Rendering/RenderCamera.h
        src/Rendering/MaterialManager.cpp
        src/ini.h
        src/Rendering/MaterialManager.h
        src/Rendering/Defines.h
        src/Rendering/EntityManager.cpp
        src/Rendering/EntityManager.h src/LuaState.cpp src/LuaState.h 
        "src/Modules/Mesh.cpp" 
        "src/Modules/Mesh.h" 
        "src/Modules/Render.h" 
        "src/Modules/Render.cpp" 
        "src/Modules/Transforms.h" 
        "src/EngineConfig.h" 
        "src/EngineConfig.cpp" 
         src/EngineUI.cpp 
         "src/Modules/Stats/Stats.h" 
         "src/Modules/Stats/Stats.cpp"
          src/Modules/Module.h)

#target_compile_features(RxEngine PUBLIC cxx_std_20)
target_compile_options(RxEngine PUBLIC -DGLFW_INCLUDE_VULKAN -DVK_USE_PLATFORM_WIN32_KHR -DGLFW_EXPOSE_NATIVE_WIN32 -DNOMINMAX)
#target_compile_options(RxEngine PUBLIC -DGLM_FORCE_DEPTH_ZERO_TO_ONE -DGLM_FORCE_XYZW_ONLY)# -DGLM_FORCE_LEFT_HANDED)
#target_compile_options(RxEngine PUBLIC -DRX_ASSET_DIR="${PROJECT_SOURCE_DIR}/assets")
#target_compile_options(RxEngine PUBLIC -DRX_ASSET_DIR="${GAME_ASSET_PATH}")

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
target_link_libraries(RxEngine PUBLIC RxCore)
target_link_libraries(RxEngine PUBLIC RxECS)
target_link_libraries(RxEngine PUBLIC RXAssets)
target_link_libraries(RxEngine PUBLIC lua_static)
target_link_libraries(RxEngine PUBLIC sol2)
#target_link_libraries(RxEngine PUBLIC LuaBridge)
target_link_libraries(RxEngine PUBLIC RmlCore)
target_link_libraries(RxEngine PUBLIC RmlDebugger)
target_link_libraries(RxEngine PUBLIC freetype)
target_link_libraries(RxEngine PUBLIC RmlLua)
target_link_libraries(RxEngine PUBLIC glfw)
target_link_libraries(RxEngine PUBLIC DirectXMath)
#target_link_libraries(RxEngine PUBLIC flecs::flecs_static)
#target_include_directories(RxEngine PUBLIC ${entt_SOURCE_DIR}/src)
#target_include_directories(RxEngine PUBLIC ${RmlUi_SOURCE_DIR}/Include)
#target_compile_options(RxEngine PRIVATE -DIMGUIZMO_IMGUI_FOLDER= -DVGIZMO_USES_GLM)
target_include_directories(RxEngine PUBLIC ${imgui_SOURCE_DIR})
#target_include_directories(RxEngine PUBLIC ${imguizmo_SOURCE_DIR}/imGuIZMO.quat)
target_sources(RxEngine PRIVATE ${imgui_SOURCE_DIR}/imgui_draw.cpp ${imgui_SOURCE_DIR}/imgui_widgets ${imgui_SOURCE_DIR}/imgui_demo.cpp ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp )
target_compile_options(RxEngine PUBLIC -DIMGUI_ENABLE_FREETYPE)
#target_sources(RxEngine PRIVATE ${imguizmo_SOURCE_DIR}/imGuIZMO.quat/imGuIZMOquat.cpp)

#target_compile_features(RxEngine PRIVATE cxx_std_17)

set_target_properties(RxEngine PROPERTIES CXX_STANDARD 20)

