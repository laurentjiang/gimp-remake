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
option(ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

add_executable(gimp-remake
    "src/main.cpp"
    "src/ui/main_window.cpp"
    "src/ui/skia_canvas_widget.cpp"
    "src/ui/toolbox_panel.cpp"
    "src/ui/tool_options_panel.cpp"
    "src/ui/tool_button.cpp"
    "src/ui/spin_slider.cpp"
    "src/ui/layers_panel.cpp"
    "src/ui/history_panel.cpp"
    "src/ui/color_chooser_panel.cpp"
    "src/ui/command_palette.cpp"
    "src/ui/debug_hud.cpp"
    "src/ui/shortcut_manager.cpp"
    "src/ui/log_message.cpp"
    "src/ui/log_sink.cpp"
    "src/ui/log_bridge.cpp"
    "src/ui/log_panel.cpp"
    "src/ui/new_document_dialog.cpp"
    "src/ui/toast_notification.cpp"
    "src/ui/toast_manager.cpp"
    "src/history/history_stack.cpp"
    "src/history/simple_history_manager.cpp"
    "src/io/io_manager.cpp"
    "src/error_handling/error_handler.cpp"
    "src/core/command_bus.cpp"
    "src/core/clipboard_manager.cpp"
    "src/core/layer_stack.cpp"
    "src/core/filters/filter.cpp"
    "src/core/filters/blur_filter.cpp"
    "src/core/filters/sharpen_filter.cpp"
    "src/core/tool.cpp"
    "src/core/tool_factory.cpp"
    "src/core/floating_buffer.cpp"
    "src/core/transform_state.cpp"
    "src/core/brush_strategy.cpp"
    "src/core/tools/pencil_tool.cpp"
    "src/core/tools/eraser_tool.cpp"
    "src/core/tools/move_tool.cpp"
    "src/core/tools/selection_tool_base.cpp"
    "src/core/tools/ellipse_selection_tool.cpp"
    "src/core/tools/rect_selection_tool.cpp"
    "src/core/tools/free_select_tool.cpp"
    "src/core/tools/color_picker_tool.cpp"
    "src/core/tools/fill_tool.cpp"
    "src/core/tools/gradient_tool.cpp"
    "src/core/tools/brush_tool.cpp"
    "src/core/commands/add_layer_command.cpp"
    "src/core/commands/fill_color_command.cpp"
    "src/core/commands/draw_command.cpp"
    "src/core/commands/move_command.cpp"
    "src/core/commands/selection_command.cpp"
    "src/core/commands/paste_command.cpp"
    "src/render/skia_renderer.cpp"
    "src/render/skia_compositor.cpp"
    "resources/resources.qrc"
    # Headers with Q_OBJECT (required for AUTOMOC when headers are in separate include/ dir)
    "include/ui/main_window.h"
    "include/ui/skia_canvas_widget.h"
    "include/ui/toolbox_panel.h"
    "include/ui/tool_options_panel.h"
    "include/ui/tool_button.h"
    "include/ui/spin_slider.h"
    "include/ui/layers_panel.h"
    "include/ui/history_panel.h"
    "include/ui/color_chooser_panel.h"
    "include/ui/command_palette.h"
    "include/ui/debug_hud.h"
    "include/ui/shortcut_manager.h"
    "include/ui/log_bridge.h"
    "include/ui/log_panel.h"
    "include/ui/new_document_dialog.h"
    "include/ui/toast_notification.h"
    "include/ui/toast_manager.h"
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
find_package(Qt6 COMPONENTS Core Gui Widgets Svg REQUIRED)
find_package(OpenCV REQUIRED)

target_link_libraries(gimp-remake PRIVATE 
    spdlog::spdlog 
    unofficial::skia::skia
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
    ${OpenCV_LIBS}
)

if(WIN32)
    # Automatically deploy Qt platform plugin for Windows
    # This ensures the app runs without setting QT_PLUGIN_PATH manually
    add_custom_command(TARGET gimp-remake POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:gimp-remake>/platforms"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:gimp-remake>/iconengines"
    )

    # Locate plugins from vcpkg installed directory using the actual triplet
    # VCPKG_TARGET_TRIPLET is set by the vcpkg toolchain file
    if(DEFINED VCPKG_TARGET_TRIPLET)
        set(_vcpkg_installed "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")
    else()
        set(_vcpkg_installed "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows")
    endif()

    # Check if this is a release-only triplet (no debug subfolder)
    # Release-only triplets like x64-windows-release have plugins directly in Qt6/plugins
    if(VCPKG_TARGET_TRIPLET MATCHES "-release$")
        # Release-only triplet: plugins are in Qt6/plugins (no debug variant)
        add_custom_command(TARGET gimp-remake POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_vcpkg_installed}/Qt6/plugins/platforms/qwindows.dll"
                "$<TARGET_FILE_DIR:gimp-remake>/platforms/qwindows.dll"
            COMMENT "Deploying Qt platform plugin"
        )
        add_custom_command(TARGET gimp-remake POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_vcpkg_installed}/Qt6/plugins/iconengines/qsvgicon.dll"
                "$<TARGET_FILE_DIR:gimp-remake>/iconengines/qsvgicon.dll"
            COMMENT "Deploying Qt SVG icon engine plugin"
        )
    else()
        # Standard triplet with debug/release: select based on build config
        add_custom_command(TARGET gimp-remake POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<IF:$<CONFIG:Debug>,${_vcpkg_installed}/debug/Qt6/plugins/platforms/qwindowsd.dll,${_vcpkg_installed}/Qt6/plugins/platforms/qwindows.dll>"
                "$<TARGET_FILE_DIR:gimp-remake>/platforms/$<IF:$<CONFIG:Debug>,qwindowsd.dll,qwindows.dll>"
            COMMENT "Deploying Qt platform plugin"
        )
        add_custom_command(TARGET gimp-remake POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<IF:$<CONFIG:Debug>,${_vcpkg_installed}/debug/Qt6/plugins/iconengines/qsvgicond.dll,${_vcpkg_installed}/Qt6/plugins/iconengines/qsvgicon.dll>"
                "$<TARGET_FILE_DIR:gimp-remake>/iconengines/$<IF:$<CONFIG:Debug>,qsvgicond.dll,qsvgicon.dll>"
            COMMENT "Deploying Qt SVG icon engine plugin"
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
        # Unit tests (pure logic, no external dependencies)
        "tests/unit/test_tool_state_machine.cpp"
        "tests/unit/test_color_picker_tool.cpp"
        "tests/unit/test_draw_command.cpp"
        "tests/unit/test_move_command.cpp"
        "tests/unit/test_selection_command.cpp"
        "tests/unit/test_clipboard_manager.cpp"
        "tests/unit/test_canvas_viewport.cpp"
        "tests/unit/test_event_bus.cpp"
        "tests/unit/test_layer_stack.cpp"
        "tests/unit/test_history_stack.cpp"
        "tests/unit/test_eraser_tool.cpp"
        "tests/unit/test_pencil_tool.cpp"
        "tests/unit/test_brush_tool.cpp"
        "tests/unit/test_fill_tool.cpp"
        "tests/unit/test_gradient_tool.cpp"
        "tests/unit/test_free_select_tool.cpp"
        "tests/unit/test_transform_state.cpp"
        "tests/unit/test_color_chooser_panel.cpp"
        "tests/unit/test_shortcut_manager.cpp"
        # Integration tests (file I/O, rendering)
        "tests/integration/test_compositor.cpp"
        "tests/integration/test_io_manager.cpp"
        # Sources needed for tests
        "src/core/layer_stack.cpp"
        "src/core/tool.cpp"
        "src/core/tool_factory.cpp"
        "src/core/clipboard_manager.cpp"
        "src/core/command_bus.cpp"
        "src/core/floating_buffer.cpp"
        "src/core/transform_state.cpp"
        "src/core/brush_strategy.cpp"
        "src/core/tools/pencil_tool.cpp"
        "src/core/tools/eraser_tool.cpp"
        "src/core/tools/move_tool.cpp"
        "src/core/tools/color_picker_tool.cpp"
        "src/core/tools/selection_tool_base.cpp"
        "src/core/tools/rect_selection_tool.cpp"
        "src/core/tools/free_select_tool.cpp"
        "src/core/tools/fill_tool.cpp"
        "src/core/tools/gradient_tool.cpp"
        "src/core/tools/brush_tool.cpp"
        "src/core/commands/draw_command.cpp"
        "src/core/commands/move_command.cpp"
        "src/core/commands/selection_command.cpp"
        "src/core/commands/paste_command.cpp"
        "src/history/history_stack.cpp"
        "src/history/simple_history_manager.cpp"
        "src/render/skia_compositor.cpp"
        "src/io/io_manager.cpp"
        "src/ui/color_chooser_panel.cpp"
        # Headers with Q_OBJECT for AUTOMOC
        "include/ui/skia_canvas_widget.h"
        "include/ui/color_chooser_panel.h"
    )

    target_include_directories(unit_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
    )

    target_link_libraries(unit_tests PRIVATE 
        Catch2::Catch2
        Catch2::Catch2WithMain 
        unofficial::skia::skia
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        spdlog::spdlog
        ${OpenCV_LIBS}
    )
    
    # Ensure tests use C++20
    target_compile_features(unit_tests PRIVATE cxx_std_20)
    
    # Define source directory for test file paths
    target_compile_definitions(unit_tests PRIVATE SOURCE_DIR="${CMAKE_SOURCE_DIR}")
    
    if(MSVC)
        target_compile_options(unit_tests PRIVATE /W4 /permissive- /Zc:__cplusplus)
    else()
        target_compile_options(unit_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    # Coverage instrumentation (Clang only)
    if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(unit_tests PRIVATE -fprofile-instr-generate -fcoverage-mapping)
        target_link_options(unit_tests PRIVATE -fprofile-instr-generate -fcoverage-mapping)
    endif()

    add_test(NAME unit_tests COMMAND unit_tests)
endif()
