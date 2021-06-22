cmake_minimum_required(VERSION 3.15)

add_library(RxUtil STATIC
    src/Log.h
    src/DampedFloat.h
    src/Pooler.h
    src/Hasher.h
    src/Log.cpp
    src/Util.h
)

if (MSVC)
    target_compile_options(RxUtil PRIVATE /W4 /wd4275 /wd4305 /wd4310 -D_CRT_SECURE_NO_WARNINGS -D_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING -DWIN32_LEAN_AND_MEAN)
endif ()

target_include_directories(RxUtil PUBLIC "src")
target_link_libraries(RxUtil PUBLIC spdlog::spdlog_header_only)
