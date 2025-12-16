cmake_minimum_required(VERSION 3.26)
project(GimpRemake LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Prefer manifest-mode vcpkg if available.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "" FORCE)
    message(STATUS "Using vcpkg toolchain from $ENV{VCPKG_ROOT}")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(GIMP_REMAKE_BUILD_TESTS "Build tests" ON)

add_executable(gimp-remake
    src/main.cpp
    core/document.h
    core/layer.h
    core/command.h
    core/history_manager.h
    core/command_bus.h
    core/command_bus.cpp
    core/commands/add_layer_command.h
    core/commands/add_layer_command.cpp
    core/commands/fill_color_command.h
    core/commands/fill_color_command.cpp
    core/event_bus.h
    core/tile_store.h
    render/renderer.h
    render/skia_renderer.h
    render/skia_renderer.cpp
)

find_package(spdlog CONFIG REQUIRED)

# Workaround for unofficial-skia warnings about missing system libs on Windows
if(WIN32)
    set(z_vcpkg_skia_link_libs_FontSub_lib_RELEASE "FontSub.lib" CACHE FILEPATH "Fix for skia warning" FORCE)
    set(z_vcpkg_skia_link_libs_FontSub_lib_DEBUG "FontSub.lib" CACHE FILEPATH "Fix for skia warning" FORCE)
    
    set(z_vcpkg_skia_link_libs_Usp10_lib_RELEASE "Usp10.lib" CACHE FILEPATH "Fix for skia warning" FORCE)
    set(z_vcpkg_skia_link_libs_Usp10_lib_DEBUG "Usp10.lib" CACHE FILEPATH "Fix for skia warning" FORCE)
endif()

find_package(unofficial-skia CONFIG REQUIRED)
target_link_libraries(gimp-remake PRIVATE spdlog::spdlog unofficial::skia::skia)

# Enable modern C++ warnings; extend per-platform as we add code.
if(MSVC)
    target_compile_options(gimp-remake PRIVATE /W4 /permissive-)
else()
    target_compile_options(gimp-remake PRIVATE -Wall -Wextra -Wpedantic)
endif()

# TODO: Wire vcpkg toolchain, Qt6 UI shell, Skia renderer, and test targets.

include(CTest)
if(GIMP_REMAKE_BUILD_TESTS)
    enable_testing()
    # Placeholder smoke test to keep CI exercising ctest until real tests land.
    add_test(NAME placeholder_smoke COMMAND ${CMAKE_COMMAND} -E echo "placeholder test")
endif()
