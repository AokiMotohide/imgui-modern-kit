cmake_minimum_required(VERSION 3.20)
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
endif()

project(imgui-modern-kit VERSION 0.2.0 LANGUAGES CXX)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(IMKIT_IS_TOP_LEVEL ON)
else()
    set(IMKIT_IS_TOP_LEVEL OFF)
endif()

option(IMKIT_BUILD_GALLERY "Build the ImKit development gallery" ${IMKIT_IS_TOP_LEVEL})
option(IMKIT_BUILD_DESIGN_GALLERY "Build the experimental theme comparison gallery" OFF)
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

add_library(imkit STATIC src/widgets.cpp src/theme.cpp src/components.cpp)
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

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
install(TARGETS imkit EXPORT imkitTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/imkit DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
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
install(FILES examples/design_gallery/assets/OFL.txt examples/design_gallery/assets/NotoSansJP-OFL.txt
    DESTINATION examples/design_gallery/assets)

if(IMKIT_BUILD_GALLERY OR IMKIT_BUILD_DESIGN_GALLERY)
    if(NOT WIN32)
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
    add_executable(imkit_gallery
        examples/gallery/main.cpp
        examples/gallery/gallery.cpp
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
            "${IMKIT_GALLERY_BACKEND_TARGET}"
            windowscodecs ole32
    )
    add_custom_command(TARGET imkit_gallery POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/examples/design_gallery/assets" "$<TARGET_FILE_DIR:imkit_gallery>/design-assets")
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
    add_custom_command(TARGET imkit_design_gallery POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/examples/design_gallery/assets"
            "$<TARGET_FILE_DIR:imkit_design_gallery>/design-assets")
endif()

if(IMKIT_BUILD_TESTS)
    include(CTest)
    add_executable(imkit_api_compile tests/api_compile.cpp)
    target_link_libraries(imkit_api_compile PRIVATE imkit::imkit)
    add_test(NAME imkit.api_compile COMMAND imkit_api_compile)

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
