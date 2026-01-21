cmake_minimum_required(VERSION 3.26)
project(GimpRemake LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Prefer manifest-mode vcpkg if available.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "" FORCE)
    message(STATUS "Using vcpkg toolchain from $ENV{VCPKG_ROOT}")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(GIMP_REMAKE_BUILD_TESTS "Build tests" ON)

add_executable(gimp-remake
    "src/main.cpp"
    "src/ui/main_window.cpp"
    "src/ui/skia_canvas_widget.cpp"
    "src/io/io_manager.cpp"
    "include/ui/main_window.h"
    "include/ui/skia_canvas_widget.h"
    "include/core/command_bus.cpp"
    "include/core/layer_stack.cpp"
    "include/core/commands/add_layer_command.cpp"
    "include/core/commands/fill_color_command.cpp"
    "include/render/skia_renderer.cpp"
    "include/render/skia_compositor.cpp"
)

target_include_directories(gimp-remake PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
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
find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)
find_package(OpenCV REQUIRED)

target_link_libraries(gimp-remake PRIVATE 
    spdlog::spdlog 
    unofficial::skia::skia
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${OpenCV_LIBS}
)

if(WIN32)
    # Automatically deploy Qt platform plugin for Windows
    # This ensures the app runs without setting QT_PLUGIN_PATH manually
    add_custom_command(TARGET gimp-remake POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:gimp-remake>/platforms"
    )

    # Attempt to locate the plugins directory relative to Qt6::Core
    # Vcpkg layout: bin/Qt6Core.dll -> ../Qt6/plugins/platforms/qwindows.dll
    get_target_property(_qt_core_lib Qt6::Core IMPORTED_LOCATION_RELEASE)
    if(NOT _qt_core_lib)
        get_target_property(_qt_core_lib Qt6::Core IMPORTED_LOCATION_DEBUG)
    endif()
    if(NOT _qt_core_lib)
        get_target_property(_qt_core_lib Qt6::Core IMPORTED_LOCATION)
    endif()

    if(_qt_core_lib)
        get_filename_component(_qt_bin_dir "${_qt_core_lib}" DIRECTORY)
        set(_qt_plugins_dir "${_qt_bin_dir}/../Qt6/plugins")
        
        # Fallback for standard Qt layout
        if(NOT EXISTS "${_qt_plugins_dir}")
            set(_qt_plugins_dir "${_qt_bin_dir}/../plugins")
        endif()

        add_custom_command(TARGET gimp-remake POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_qt_plugins_dir}/platforms/qwindows.dll"
                "$<TARGET_FILE_DIR:gimp-remake>/platforms/qwindows.dll"
            COMMENT "Deploying Qt platform plugin (qwindows.dll)"
        )
    endif()
endif()

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
    
    # Use FetchContent for Catch2 to ensure compiler compatibility and avoid ABI mismatches
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.2
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(unit_tests
        "tests/test_compositor.cpp"
        "include/core/layer_stack.cpp"
        "include/render/skia_compositor.cpp"
    )

    target_include_directories(unit_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(unit_tests PRIVATE 
        Catch2::Catch2
        Catch2::Catch2WithMain 
        unofficial::skia::skia
        Qt6::Core
        spdlog::spdlog
    )
    
    # Ensure tests use C++20
    target_compile_features(unit_tests PRIVATE cxx_std_20)
    
    if(MSVC)
        target_compile_options(unit_tests PRIVATE /W4 /permissive- /Zc:__cplusplus)
    else()
        target_compile_options(unit_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME unit_tests COMMAND unit_tests)
endif()
