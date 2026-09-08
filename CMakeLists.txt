cmake_minimum_required(VERSION 3.20)

project(imgui-modern-kit VERSION 0.1.0 LANGUAGES CXX)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(IMKIT_IS_TOP_LEVEL ON)
else()
    set(IMKIT_IS_TOP_LEVEL OFF)
endif()

option(IMKIT_BUILD_GALLERY "Build the ImKit development gallery" ${IMKIT_IS_TOP_LEVEL})
option(IMKIT_BUILD_TESTS "Build the ImKit foundation tests" ${IMKIT_IS_TOP_LEVEL})

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

add_library(imkit STATIC src/widgets.cpp)
add_library(imkit::imkit ALIAS imkit)

target_compile_features(imkit PUBLIC cxx_std_20)
target_include_directories(imkit
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)
target_link_libraries(imkit PUBLIC "${IMKIT_IMGUI_TARGET}")

if(IMKIT_BUILD_GALLERY)
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

    add_executable(imkit_gallery
        examples/gallery/main.cpp
        examples/gallery/gallery.cpp
        examples/gallery/gallery.h
    )
    target_compile_features(imkit_gallery PRIVATE cxx_std_20)
    target_include_directories(imkit_gallery PRIVATE examples/gallery)
    target_link_libraries(imkit_gallery
        PRIVATE
            imkit::imkit
            "${IMKIT_GALLERY_BACKEND_TARGET}"
    )
endif()

if(IMKIT_BUILD_TESTS)
    include(CTest)

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

