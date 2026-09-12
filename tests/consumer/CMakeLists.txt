cmake_minimum_required(VERSION 3.20)

project(imkit-consumer LANGUAGES CXX)

if(NOT DEFINED IMKIT_SOURCE_DIR OR NOT DEFINED IMGUI_SOURCE_DIR)
    message(FATAL_ERROR "IMKIT_SOURCE_DIR and IMGUI_SOURCE_DIR are required.")
endif()

add_library(consumer_imgui STATIC
    "${IMGUI_SOURCE_DIR}/imgui.cpp"
    "${IMGUI_SOURCE_DIR}/imgui_draw.cpp"
    "${IMGUI_SOURCE_DIR}/imgui_tables.cpp"
    "${IMGUI_SOURCE_DIR}/imgui_widgets.cpp"
)
target_compile_features(consumer_imgui PUBLIC cxx_std_20)
target_include_directories(consumer_imgui PUBLIC "${IMGUI_SOURCE_DIR}")

set(IMKIT_IMGUI_TARGET consumer_imgui)
set(IMKIT_BUILD_GALLERY OFF)
set(IMKIT_BUILD_TESTS OFF)
set(IMKIT_BUILD_DESIGN_GALLERY OFF)
if(WIN32)
    set(IMKIT_BUILD_WINDOW_FRAME_WIN32 ON)
endif()
if(APPLE)
    set(IMKIT_BUILD_WINDOW_FRAME_MACOS ON)
endif()
if(IMKIT_PACKAGE_DIR)
    find_package(imkit 2.0 CONFIG REQUIRED PATHS "${IMKIT_PACKAGE_DIR}" NO_DEFAULT_PATH)
else()
    add_subdirectory("${IMKIT_SOURCE_DIR}" imkit-subbuild)
endif()

if(TARGET glfw OR TARGET OpenGL::GL OR TARGET imkit_gallery_backends)
    message(FATAL_ERROR "Library-only consumption unexpectedly created a gallery dependency target.")
endif()

if(NOT IMKIT_PACKAGE_DIR)
get_target_property(imkit_sources imkit SOURCES)
foreach(source IN LISTS imkit_sources)
    if(source MATCHES "(^|/)(imgui[^/]*|.*backend.*)\\.cpp$")
        message(FATAL_ERROR "The imkit library must not compile Dear ImGui or backend sources.")
    endif()
endforeach()
endif()

add_executable(imkit_consumer main.cpp)
target_compile_features(imkit_consumer PRIVATE cxx_std_20)
target_link_libraries(imkit_consumer PRIVATE imkit::imkit)
imkit_copy_font_assets(imkit_consumer "fonts")
add_executable(imkit_window_frame_consumer "${IMKIT_SOURCE_DIR}/tests/window_frame_api_compile.cpp")
target_link_libraries(imkit_window_frame_consumer PRIVATE imkit::imkit)
if(TARGET imkit::window_frame_win32)
    target_link_libraries(imkit_window_frame_consumer PRIVATE imkit::window_frame_win32)
endif()
if(TARGET imkit::window_frame_macos)
    target_link_libraries(imkit_window_frame_consumer PRIVATE imkit::window_frame_macos)
endif()
add_executable(imkit_workflow_consumer "${IMKIT_SOURCE_DIR}/tests/workflow_api_compile.cpp")
target_link_libraries(imkit_workflow_consumer PRIVATE imkit::editor_core)
add_executable(imkit_editor_consumer "${IMKIT_SOURCE_DIR}/tests/editor_api_compile.cpp")
target_link_libraries(imkit_editor_consumer PRIVATE imkit::editor_core imkit::video imkit::cg imkit::preview_opengl3 imkit::editor_suite)

add_executable(imkit_node_editor_consumer "${IMKIT_SOURCE_DIR}/tests/node_editor_api_compile.cpp")
target_link_libraries(imkit_node_editor_consumer PRIVATE imkit::node_editor)
