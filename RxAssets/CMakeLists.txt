cmake_minimum_required(VERSION 3.15)

add_library(RxAssets STATIC
        src/RXAssets.h
        src/SerialisationData.h
        src/dds-ktx.h
        src/Vfs.cpp
        src/Vfs.h
        src/miniz.c
        src/miniz.h
        src/Loader.cpp
        src/tser.hpp
        src/Loader.h
        src/AssetException.cpp
        src/stb_image.cpp "src/tser.hpp")

target_compile_features(RxAssets PUBLIC cxx_std_20)

if (MSVC)
    target_compile_options(RxAssets PRIVATE /W4 /WX /wd4100 -D_CRT_SECURE_NO_WARNINGS -D_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING -DWIN32_LEAN_AND_MEAN)
    #set_source_files_properties(src/miniz.c PROPERTIES COMPILE_FLAGS /wd4100)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set_source_files_properties(src/stb_image.cpp PROPERTIES COMPILE_FLAGS -Wno-missing-field-initializers )
    endif()
endif ()

#target_compile_options(RXAssets PUBLIC -DSPDLOG_ACTIVE_LEVEL=0)

target_include_directories(RxAssets PUBLIC "src")

#target_link_libraries(RXAssets PUBLIC spdlog::spdlog_header_only)
#target_link_libraries(RXAssets PUBLIC glm)
target_link_libraries(RxAssets PUBLIC nlohmann_json nlohmann_json::nlohmann_json)
#target_link_libraries(RxAssets PUBLIC Bitsery::bitsery)
target_link_libraries(RxAssets PRIVATE RxUtil)
#target_include_directories(RXAssets PUBLIC ${entt_SOURCE_DIR}/src)

target_include_directories(RxAssets PRIVATE ${Vulkan_INCLUDE_DIR})
#target_include_directories(RXAssets PRIVATE ${tinygltf_SOURCE_DIR})

set_target_properties(RxAssets PROPERTIES
        CXX_STANDARD 20
        CXX_EXTENSIONS ON
        )
