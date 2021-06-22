cmake_minimum_required(VERSION 3.15)

find_package(Vulkan REQUIRED)

add_library(RxCore STATIC 
    src/Vulkan/Device.cpp
    src/Vulkan/Device.h
    src/Vulkan/CommandBuffer.cpp
    src/Vulkan/CommandBuffer.hpp
    src/Vulkan/Vulk.hpp
    src/Vulkan/Buffer.cpp
    src/Vulkan/Buffer.hpp
    src/Vulkan/VertexBuffer.cpp
    src/Vulkan/VertexBuffer.h
    src/Vulkan/IndexBuffer.hpp
    src/Vulkan/Image.hpp
    src/Vulkan/Image.cpp
    src/Vulkan/DescriptorSet.hpp
    src/Vulkan/DescriptorSet.cpp
    src/Vulkan/DescriptorPool.cpp
    src/Vulkan/DescriptorPool.hpp
    src/Vulkan/Queue.cpp
    src/Vulkan/Queue.hpp
    src/Vulkan/CommandPool.cpp
    src/Vulkan/CommandPool.hpp
    src/Vulkan/VMA.cpp 
    src/RXCore.h
    src/optick/optick_core.cpp
    src/optick/optick_server.cpp
    src/optick/optick_serialization.cpp
    src/optick/optick_miniz.cpp
    src/optick/optick_message.cpp
    src/optick/optick_gpu.cpp
    src/optick/optick_gpu.vulkan.cpp
    src/Jobs/JobManager.cpp
    src/Jobs/JobManager.hpp
    src/Vulkan/FrameBuffer.h
    src/Vulkan/Shader.h
    src/Vulkan/Allocation.cpp
    src/Vulkan/Allocation.h
    src/Vulkan/ThreadResources.cpp
    src/Vulkan/ThreadResources.h
    src/Vulkan/Pipeline.h 
    src/Window.cpp
    src/Window.hpp
    src/Events.h
    src/Events.cpp
    src/Vulkan/VkBootstrap.cpp
    #src/Vulkan/volk.c
)

target_compile_options(RxCore PUBLIC -DNOMINMAX)
target_compile_options(RxCore PUBLIC -DUSE_OPTICK=1)
target_compile_options(RxCore PUBLIC -DVK_USE_PLATFORM_WIN32_KHR)
#target_compile_options(RxCore PUBLIC -DVULKAN_HPP_NO_SPACESHIP_OPERATOR=1)

if (MSVC)
    target_compile_options(RxCore PRIVATE /arch:AVX2 /W4 /WX /wd4100 /EHsc)
    target_compile_options(RxCore PUBLIC -D_CRT_SECURE_NO_WARNINGS -D_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING -DWIN32_LEAN_AND_MEAN)
    set_source_files_properties(src/Vulkan/VMA.cpp PROPERTIES COMPILE_FLAGS /W3 )
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(RxCore PRIVATE -Werror)
        target_compile_options(RxCore PRIVATE -Wno-missing-field-initializers)
        target_compile_options(RxCore PRIVATE -Wno-unused-variable)
        target_compile_options(RxCore PRIVATE -Wno-unused-function)
    set_source_files_properties(src/Vulkan/VMA.cpp PROPERTIES COMPILE_FLAGS -Wno-unused-function )
    endif()
endif ()

#target_compile_options(RxCore PUBLIC -DVULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
target_include_directories(RxCore PUBLIC "src")
target_include_directories(RxCore INTERFACE "include")
target_include_directories(RxCore PUBLIC ${Vulkan_INCLUDE_DIR})
target_link_libraries(RxCore PUBLIC ${Vulkan_LIBRARY})
target_link_libraries(RxCore PUBLIC RxUtil)
target_link_libraries(RxCore PUBLIC sdl2)

set_target_properties(RxCore PROPERTIES CXX_STANDARD 20)
