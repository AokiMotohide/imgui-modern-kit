cmake_minimum_required(VERSION 3.20)
project(imkit-example LANGUAGES CXX)
if(NOT IMGUI_SOURCE_DIR)
    message(FATAL_ERROR "Set IMGUI_SOURCE_DIR to Dear ImGui 1.92.9b docking.")
endif()
add_library(host_imgui STATIC
    ${IMGUI_SOURCE_DIR}/imgui.cpp
    ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
    ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
    ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp)
target_include_directories(host_imgui PUBLIC ${IMGUI_SOURCE_DIR})
set(IMKIT_IMGUI_TARGET host_imgui)
if(IMKIT_SOURCE_DIR)
    set(IMKIT_BUILD_GALLERY OFF)
    set(IMKIT_BUILD_DESIGN_GALLERY OFF)
    set(IMKIT_BUILD_TESTS OFF)
    add_subdirectory(${IMKIT_SOURCE_DIR} imkit-build)
else()
    find_package(imkit 1.0 CONFIG REQUIRED)
endif()
add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE imkit::imkit)
