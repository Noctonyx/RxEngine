# CMakeList.txt : CMake project for RX, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

# Add source to this project's executable.
add_executable (ImportGLTF 
    Main.cpp
 "RXAssetManager.h" "gltf.cpp")

if (MSVC)
    target_compile_options(ImportGLTF PRIVATE /W3 /wd4275 /wd4305 /wd4310)
    target_compile_options(ImportGLTF PUBLIC /DTINYGLTF_NO_INCLUDE_STB_IMAGE)
    target_compile_options(ImportGLTF PUBLIC /DTINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE)
    #target_compile_options(ImportGLTF PUBLIC /DTINYGLTF_NO_STB_IMAGE)
endif ()

target_link_libraries(ImportGLTF PRIVATE RxAssets)
#target_link_libraries(RXAssetManager PUBLIC spdlog::spdlog_header_only)z
#target_link_libraries(RXAssetManager PUBLIC glm)
#target_link_libraries(ImportGLTF PUBLIC nlohmann_json nlohmann_json::nlohmann_json)
#target_link_libraries(ImportGLTF PRIVATE Bitsery::bitsery)
#target_link_libraries(RXAssetManager PRIVATE spirv-cross-cpp)
#target_link_libraries(RXAssetManager PRIVATE spirv-cross-glsl)
#target_link_libraries(RXAssetManager PRIVATE spirv-cross-reflect)
#target_link_libraries(RXAssetManager PRIVATE glslang)
#target_link_libraries(RXAssetManager PRIVATE SPIRV)
target_link_libraries(ImportGLTF PRIVATE RxUtil)

#target_include_directories(RXAssetManager PRIVATE ${Vulkan_INCLUDE_DIR})
#target_link_libraries(RXAssetManager PRIVATE RXEngine)
##arget_include_directories(RXAssetManager PUBLIC ${entt_SOURCE_DIR}/src)
target_include_directories(ImportGLTF PRIVATE ${tinygltf_SOURCE_DIR})

#set(RXASSETMANAGER_EXE "${CMAKE_CURRENT_BINARY_DIR}/RXAssetManager${CMAKE_EXECUTABLE_SUFFIX}")
#message (" VARIABLE = ${RXASSETMANAGER_EXE}")

#add_custom_command(TARGET ImportGLTF POST_BUILD 
    #COMMAND ${CMAKE_COMMAND} -E copy "${RXASSETMANAGER_EXE}" "D:/RXAssets")
