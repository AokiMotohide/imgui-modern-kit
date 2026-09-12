cmake_minimum_required(VERSION 3.20)
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
endif()

project(imgui-modern-kit VERSION 2.2.0 LANGUAGES CXX)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(IMKIT_IS_TOP_LEVEL ON)
else()
    set(IMKIT_IS_TOP_LEVEL OFF)
endif()

option(IMKIT_BUILD_GALLERY "Build the ImKit development gallery" ${IMKIT_IS_TOP_LEVEL})
option(IMKIT_BUILD_DESIGN_GALLERY "Build the experimental theme comparison gallery" OFF)
option(IMKIT_BUILD_TESTS "Build the ImKit foundation tests" ${IMKIT_IS_TOP_LEVEL})
if(WIN32 AND IMKIT_IS_TOP_LEVEL)
    set(IMKIT_WINDOW_FRAME_WIN32_DEFAULT ON)
else()
    set(IMKIT_WINDOW_FRAME_WIN32_DEFAULT OFF)
endif()
if(APPLE AND IMKIT_IS_TOP_LEVEL)
    set(IMKIT_WINDOW_FRAME_MACOS_DEFAULT ON)
else()
    set(IMKIT_WINDOW_FRAME_MACOS_DEFAULT OFF)
endif()
option(IMKIT_BUILD_WINDOW_FRAME_WIN32 "Build the borrowed HWND window-frame adapter" ${IMKIT_WINDOW_FRAME_WIN32_DEFAULT})
option(IMKIT_BUILD_WINDOW_FRAME_MACOS "Build the borrowed NSWindow window-frame adapter" ${IMKIT_WINDOW_FRAME_MACOS_DEFAULT})
option(IMKIT_BUILD_WINDOW_FRAME_DEMO_MACOS "Build the lightweight macOS window-frame demo" OFF)
option(IMKIT_BUILD_WINDOW_FRAME_LEGACY_IMGUI "Build only the window-frame API for the supported legacy Dear ImGui 1.88 host" OFF)

set(IMKIT_FONT_ASSET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts"
    CACHE INTERNAL "ImKit optional host font assets" FORCE)
include(cmake/imkitFontAssets.cmake)

if(NOT DEFINED IMKIT_IMGUI_TARGET)
    set(IMKIT_IMGUI_TARGET "" CACHE STRING "Existing Dear ImGui CMake target used by ImKit")
endif()

include(cmake/Dependencies.cmake)

set(IMKIT_USES_BUNDLED_IMGUI OFF)
set(IMKIT_IMGUI_SOURCE_DIR "")

if(IMKIT_IMGUI_TARGET STREQUAL "")
    if(NOT IMKIT_IS_TOP_LEVEL)
        message(FATAL_ERROR
            "IMKIT_IMGUI_TARGET is required when imgui-modern-kit is added as a subdirectory. "
            "Create the host Dear ImGui target first and set IMKIT_IMGUI_TARGET to its name.")
    endif()

    imkit_provide_bundled_imgui(IMKIT_IMGUI_TARGET IMKIT_IMGUI_SOURCE_DIR)
    set(IMKIT_USES_BUNDLED_IMGUI ON)
endif()

if(NOT TARGET "${IMKIT_IMGUI_TARGET}")
    message(FATAL_ERROR
        "IMKIT_IMGUI_TARGET names '${IMKIT_IMGUI_TARGET}', but no such CMake target exists. "
        "Create the Dear ImGui target before adding imgui-modern-kit.")
endif()

add_library(imkit STATIC src/widgets.cpp src/theme.cpp src/components.cpp src/icons.cpp src/accessibility.cpp src/patterns.cpp src/workflow.cpp src/shell.cpp src/window_frame.cpp)
add_library(imkit::imkit ALIAS imkit)

target_compile_features(imkit PUBLIC cxx_std_20)
target_include_directories(imkit
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(imkit PUBLIC "$<BUILD_INTERFACE:${IMKIT_IMGUI_TARGET}>" "$<INSTALL_INTERFACE:imkit::host_imgui>")
set_target_properties(imkit PROPERTIES DEBUG_POSTFIX d)
set_property(TARGET imkit PROPERTY MSVC_DEBUG_INFORMATION_FORMAT Embedded)
target_compile_options(imkit PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
target_compile_options(imkit PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/pathmap:${CMAKE_CURRENT_SOURCE_DIR}=imkit>")
target_compile_options(imkit PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/experimental:deterministic>)
if(MSVC)
    get_filename_component(IMKIT_MSVC_ROOT "${CMAKE_CXX_COMPILER}/../../../.." ABSOLUTE)
    target_compile_options(imkit PRIVATE "/pathmap:${IMKIT_MSVC_ROOT}=toolchain/msvc")
    get_filename_component(IMKIT_WINDOWS_KITS_ROOT
        "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]" ABSOLUTE)
    if(EXISTS "${IMKIT_WINDOWS_KITS_ROOT}/Include")
        target_compile_options(imkit PRIVATE "/pathmap:${IMKIT_WINDOWS_KITS_ROOT}=toolchain/windows-sdk")
    endif()
endif()

if(IMKIT_BUILD_WINDOW_FRAME_LEGACY_IMGUI)
    add_library(imkit_window_frame_legacy STATIC src/window_frame.cpp)
    add_library(imkit::window_frame_legacy ALIAS imkit_window_frame_legacy)
    target_compile_features(imkit_window_frame_legacy PUBLIC cxx_std_20)
    target_include_directories(imkit_window_frame_legacy PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:include>")
    target_link_libraries(imkit_window_frame_legacy PUBLIC "${IMKIT_IMGUI_TARGET}")
    target_compile_definitions(imkit_window_frame_legacy PUBLIC IMKIT_WINDOW_FRAME_LEGACY_IMGUI=1)
    target_compile_options(imkit_window_frame_legacy PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
    set_target_properties(imkit_window_frame_legacy PROPERTIES DEBUG_POSTFIX d)
    if(WIN32)
        add_library(imkit_window_frame_win32_legacy STATIC src/window_frame_win32.cpp)
        add_library(imkit::window_frame_win32_legacy ALIAS imkit_window_frame_win32_legacy)
        target_link_libraries(imkit_window_frame_win32_legacy PUBLIC imkit::window_frame_legacy PRIVATE comctl32 dwmapi user32)
        target_compile_definitions(imkit_window_frame_win32_legacy PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
        set_target_properties(imkit_window_frame_win32_legacy PROPERTIES DEBUG_POSTFIX d)
    endif()
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
option(IMKIT_BUILD_ACCESSIBILITY_WIN32 "Build the host-owned HWND UI Automation adapter" OFF)
if(IMKIT_BUILD_ACCESSIBILITY_WIN32)
    if(NOT WIN32)
        message(FATAL_ERROR "IMKIT_BUILD_ACCESSIBILITY_WIN32 requires Windows")
    endif()
    add_library(imkit_accessibility_win32 STATIC src/accessibility_win32.cpp)
    add_library(imkit::accessibility_win32 ALIAS imkit_accessibility_win32)
    set_target_properties(imkit_accessibility_win32 PROPERTIES EXPORT_NAME accessibility_win32 DEBUG_POSTFIX d)
    target_link_libraries(imkit_accessibility_win32 PUBLIC imkit::imkit PRIVATE uiautomationcore oleaut32 user32)
    target_compile_definitions(imkit_accessibility_win32 PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    install(TARGETS imkit_accessibility_win32 EXPORT imkitTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
if(IMKIT_BUILD_WINDOW_FRAME_WIN32)
    if(NOT WIN32)
        message(FATAL_ERROR "IMKIT_BUILD_WINDOW_FRAME_WIN32 requires Windows")
    endif()
    add_library(imkit_window_frame_win32 STATIC src/window_frame_win32.cpp)
    add_library(imkit::window_frame_win32 ALIAS imkit_window_frame_win32)
    set_target_properties(imkit_window_frame_win32 PROPERTIES EXPORT_NAME window_frame_win32 DEBUG_POSTFIX d)
    target_link_libraries(imkit_window_frame_win32 PUBLIC imkit::imkit PRIVATE comctl32 dwmapi user32)
    target_compile_definitions(imkit_window_frame_win32 PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    install(TARGETS imkit_window_frame_win32 EXPORT imkitTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(FILES include/imkit/window_frame_win32.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/imkit)
endif()
if(IMKIT_BUILD_WINDOW_FRAME_MACOS)
    if(NOT APPLE)
        message(FATAL_ERROR "IMKIT_BUILD_WINDOW_FRAME_MACOS requires macOS")
    endif()
    enable_language(OBJCXX)
    add_library(imkit_window_frame_macos STATIC src/window_frame_macos.mm)
    add_library(imkit::window_frame_macos ALIAS imkit_window_frame_macos)
    set_target_properties(imkit_window_frame_macos PROPERTIES EXPORT_NAME window_frame_macos DEBUG_POSTFIX d)
    target_link_libraries(imkit_window_frame_macos PUBLIC imkit::imkit PRIVATE "-framework Cocoa")
    install(TARGETS imkit_window_frame_macos EXPORT imkitTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(FILES include/imkit/window_frame_macos.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/imkit)
endif()
add_library(imkit_editor_core STATIC src/editor_core.cpp src/editor_canvas.cpp)
add_library(imkit::editor_core ALIAS imkit_editor_core)
set_target_properties(imkit_editor_core PROPERTIES EXPORT_NAME editor_core DEBUG_POSTFIX d)
target_link_libraries(imkit_editor_core PUBLIC imkit::imkit)
target_compile_options(imkit_editor_core PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
install(TARGETS imkit_editor_core EXPORT imkitTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
add_library(imkit_video STATIC src/video.cpp)
add_library(imkit::video ALIAS imkit_video)
set_target_properties(imkit_video PROPERTIES EXPORT_NAME video DEBUG_POSTFIX d)
target_link_libraries(imkit_video PUBLIC imkit::editor_core)
target_compile_options(imkit_video PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
install(TARGETS imkit_video EXPORT imkitTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
add_library(imkit_cg STATIC src/cg.cpp src/preview.cpp)
add_library(imkit::cg ALIAS imkit_cg)
set_target_properties(imkit_cg PROPERTIES EXPORT_NAME cg DEBUG_POSTFIX d)
target_link_libraries(imkit_cg PUBLIC imkit::editor_core)
target_compile_options(imkit_cg PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
add_library(imkit_editor_suite INTERFACE)
add_library(imkit::editor_suite ALIAS imkit_editor_suite)
set_target_properties(imkit_editor_suite PROPERTIES EXPORT_NAME editor_suite)
target_link_libraries(imkit_editor_suite INTERFACE imkit::editor_core imkit::video imkit::cg)
install(TARGETS imkit_cg imkit_editor_suite EXPORT imkitTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
add_library(imkit_preview_opengl3 STATIC src/preview_opengl3.cpp)
add_library(imkit::preview_opengl3 ALIAS imkit_preview_opengl3)
set_target_properties(imkit_preview_opengl3 PROPERTIES EXPORT_NAME preview_opengl3 DEBUG_POSTFIX d)
target_link_libraries(imkit_preview_opengl3 PUBLIC imkit::cg)
install(TARGETS imkit_preview_opengl3 EXPORT imkitTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS imkit EXPORT imkitTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/imkit DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    PATTERN "window_frame_win32.h" EXCLUDE
    PATTERN "window_frame_macos.h" EXCLUDE)
install(EXPORT imkitTargets NAMESPACE imkit:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imkit)
configure_package_config_file(cmake/imkitConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/imkitConfig.cmake" INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imkit)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/imkitConfigVersion.cmake" VERSION ${PROJECT_VERSION} COMPATIBILITY SameMinorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/imkitConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/imkitConfigVersion.cmake" DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imkit)
install(FILES README.md README.ja.md LICENSE THIRD_PARTY_NOTICES.md CHANGELOG.md DESTINATION .)
install(DIRECTORY docs/ DESTINATION docs
    PATTERN "precision-layers-implementation-prompt.md" EXCLUDE
    PATTERN "completion-plan.md" EXCLUDE
    PATTERN "design-proposals.md" EXCLUDE
    PATTERN "design-refinements.md" EXCLUDE)
install(DIRECTORY examples/consumer DESTINATION examples)
install(DIRECTORY assets/icons/ DESTINATION share/imkit/icons
    PATTERN "originals" EXCLUDE PATTERN "rejected" EXCLUDE)
install(DIRECTORY assets/fonts/ DESTINATION share/imkit/fonts)
install(FILES cmake/imkitFontAssets.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imkit)
install(FILES examples/design_gallery/assets/OFL.txt examples/design_gallery/assets/NotoSansJP-OFL.txt
    DESTINATION examples/design_gallery/assets)

if(IMKIT_BUILD_GALLERY OR IMKIT_BUILD_DESIGN_GALLERY OR IMKIT_BUILD_WINDOW_FRAME_DEMO_MACOS)
    if((IMKIT_BUILD_GALLERY OR IMKIT_BUILD_DESIGN_GALLERY) AND NOT WIN32)
        message(FATAL_ERROR "Catalog capture hosts currently require Windows. Disable catalog targets for library-only builds.")
    endif()
    if(IMKIT_IMGUI_SOURCE_DIR STREQUAL "")
        imkit_find_imgui_source_dir_from_target(
            "${IMKIT_IMGUI_TARGET}"
            IMKIT_IMGUI_SOURCE_DIR
        )
    endif()

    if(IMKIT_IMGUI_SOURCE_DIR STREQUAL "")
        message(FATAL_ERROR
            "IMKIT_BUILD_GALLERY requires access to the official Dear ImGui backend sources. "
            "The source directory could not be inferred from IMKIT_IMGUI_TARGET include paths.")
    endif()

    imkit_provide_gallery_dependencies(
        "${IMKIT_IMGUI_TARGET}"
        "${IMKIT_IMGUI_SOURCE_DIR}"
        IMKIT_GALLERY_BACKEND_TARGET
    )
endif()

if(IMKIT_BUILD_GALLERY)
    if(NOT TARGET imkit_window_frame_win32)
        message(FATAL_ERROR "IMKIT_BUILD_GALLERY requires IMKIT_BUILD_WINDOW_FRAME_WIN32")
    endif()
    add_executable(imkit_gallery
        examples/gallery/main.cpp
        examples/gallery/gallery.cpp
        examples/gallery/design_pages.cpp
        examples/gallery/workflow_pages.cpp
        examples/gallery/editor_workspaces.cpp
        examples/gallery/allocation_probe.cpp
        examples/gallery/gallery.h
        examples/design_gallery/capture.cpp
    )
    target_compile_features(imkit_gallery PRIVATE cxx_std_20)
    set_target_properties(imkit_gallery PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/catalog")
    target_compile_options(imkit_gallery PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
    target_compile_definitions(imkit_gallery PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_include_directories(imkit_gallery PRIVATE examples/gallery)
    target_link_libraries(imkit_gallery
        PRIVATE
            imkit::imkit
            imkit::editor_suite
            imkit::preview_opengl3
            imkit::window_frame_win32
            "${IMKIT_GALLERY_BACKEND_TARGET}"
            windowscodecs ole32 comctl32 dwmapi
    )
    imkit_copy_font_assets(imkit_gallery "design-assets")
endif()

if(IMKIT_BUILD_WINDOW_FRAME_DEMO_MACOS)
    if(NOT APPLE OR NOT TARGET imkit_window_frame_macos)
        message(FATAL_ERROR "IMKIT_BUILD_WINDOW_FRAME_DEMO_MACOS requires macOS and IMKIT_BUILD_WINDOW_FRAME_MACOS")
    endif()
    add_executable(imkit_window_frame_demo_macos examples/window_frame_macos/main.mm)
    target_compile_features(imkit_window_frame_demo_macos PRIVATE cxx_std_20)
    target_link_libraries(imkit_window_frame_demo_macos PRIVATE
        imkit::imkit imkit::window_frame_macos "${IMKIT_GALLERY_BACKEND_TARGET}" "-framework Cocoa")
endif()

if(IMKIT_BUILD_DESIGN_GALLERY)
    if(NOT WIN32)
        message(FATAL_ERROR "The experimental capture host currently requires Windows WIC.")
    endif()
    add_executable(imkit_design_gallery
        examples/design_gallery/main.cpp
        examples/design_gallery/design_lab.cpp
        examples/design_gallery/capture.cpp
    )
    target_compile_features(imkit_design_gallery PRIVATE cxx_std_20)
    target_compile_options(imkit_design_gallery PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
    target_compile_definitions(imkit_design_gallery PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(imkit_design_gallery PRIVATE imkit::imkit
        "${IMKIT_GALLERY_BACKEND_TARGET}" windowscodecs ole32)
    imkit_copy_font_assets(imkit_design_gallery "design-assets")
endif()

if(IMKIT_BUILD_TESTS)
    include(CTest)
    add_executable(imkit_workflow_test tests/workflow.cpp examples/gallery/allocation_probe.cpp)
    target_link_libraries(imkit_workflow_test PRIVATE imkit::editor_core)
    add_test(NAME imkit.workflow COMMAND imkit_workflow_test)
    add_executable(imkit_workflow_api_compile tests/workflow_api_compile.cpp)
    target_link_libraries(imkit_workflow_api_compile PRIVATE imkit::editor_core)
    add_test(NAME imkit.workflow_api_compile COMMAND imkit_workflow_api_compile)
    add_executable(imkit_design_system_test tests/design_system.cpp examples/gallery/allocation_probe.cpp)
    target_link_libraries(imkit_design_system_test PRIVATE imkit::imkit)
    add_test(NAME imkit.design_system COMMAND imkit_design_system_test)
    add_executable(imkit_design_api_compile tests/design_api_compile.cpp)
    target_link_libraries(imkit_design_api_compile PRIVATE imkit::imkit)
    add_test(NAME imkit.design_api_compile COMMAND imkit_design_api_compile)
    if(IMKIT_BUILD_ACCESSIBILITY_WIN32)
        add_executable(imkit_accessibility_win32_test tests/accessibility_win32.cpp)
        target_link_libraries(imkit_accessibility_win32_test PRIVATE imkit::accessibility_win32 ole32)
        add_test(NAME imkit.accessibility_win32 COMMAND imkit_accessibility_win32_test)
    endif()
    add_executable(imkit_editor_core_test tests/editor_core.cpp)
    target_link_libraries(imkit_editor_core_test PRIVATE imkit::editor_core)
    add_test(NAME imkit.editor_core COMMAND imkit_editor_core_test)
    add_executable(imkit_video_test tests/video.cpp examples/gallery/allocation_probe.cpp)
    target_link_libraries(imkit_video_test PRIVATE imkit::video)
    add_test(NAME imkit.video COMMAND imkit_video_test)
    add_executable(imkit_cg_test tests/cg.cpp)
    target_link_libraries(imkit_cg_test PRIVATE imkit::cg)
    add_test(NAME imkit.cg COMMAND imkit_cg_test)
    add_executable(imkit_editor_api_compile tests/editor_api_compile.cpp)
    target_link_libraries(imkit_editor_api_compile PRIVATE imkit::editor_suite imkit::preview_opengl3)
    add_test(NAME imkit.editor_api_compile COMMAND imkit_editor_api_compile)
    add_executable(imkit_icons_test tests/icons.cpp)
    target_link_libraries(imkit_icons_test PRIVATE imkit::imkit)
    add_test(NAME imkit.icons COMMAND imkit_icons_test)
    set_tests_properties(imkit.icons PROPERTIES TIMEOUT 30)
    add_executable(imkit_api_compile tests/api_compile.cpp)
    target_link_libraries(imkit_api_compile PRIVATE imkit::imkit)
    add_test(NAME imkit.api_compile COMMAND imkit_api_compile)
    add_executable(imkit_window_frame_test tests/window_frame.cpp)
    target_link_libraries(imkit_window_frame_test PRIVATE imkit::imkit)
    add_test(NAME imkit.window_frame COMMAND imkit_window_frame_test)
    add_executable(imkit_window_frame_api_compile tests/window_frame_api_compile.cpp)
    target_link_libraries(imkit_window_frame_api_compile PRIVATE imkit::imkit)
    if(TARGET imkit_window_frame_win32)
        target_link_libraries(imkit_window_frame_api_compile PRIVATE imkit::window_frame_win32)
    endif()
    if(TARGET imkit_window_frame_macos)
        target_link_libraries(imkit_window_frame_api_compile PRIVATE imkit::window_frame_macos)
    endif()
    add_test(NAME imkit.window_frame_api_compile COMMAND imkit_window_frame_api_compile)

    add_executable(imkit_theme_test tests/theme.cpp)
    target_link_libraries(imkit_theme_test PRIVATE imkit::imkit)
    add_test(NAME imkit.theme COMMAND imkit_theme_test)

    add_executable(imkit_context_smoke tests/context_smoke.cpp)
    target_compile_features(imkit_context_smoke PRIVATE cxx_std_20)
    target_link_libraries(imkit_context_smoke PRIVATE imkit::imkit)

    add_test(NAME imkit.context_smoke COMMAND imkit_context_smoke)

    configure_file(
        tests/consumer/run_consumer.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/run_consumer.cmake"
        @ONLY
    )
    add_test(
        NAME imkit.consumer_build
        COMMAND "${CMAKE_COMMAND}"
            "-DIMKIT_CONSUMER_CONFIG=$<CONFIG>"
            -P "${CMAKE_CURRENT_BINARY_DIR}/run_consumer.cmake"
    )
endif()

include(cmake/imkitNodeEditor.cmake)
