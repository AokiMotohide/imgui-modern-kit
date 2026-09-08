include(FetchContent)

set(IMKIT_IMGUI_COMMIT "b48d1afbe8ee8b238e2961dc363a949dd7304e23")
set(IMKIT_GLFW_COMMIT "d9d6f0f1f967807ffade6598ea9a631ebaf37a56")

function(imkit_provide_bundled_imgui out_target out_source_dir)
    FetchContent_Declare(
        imkit_imgui_source
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG "${IMKIT_IMGUI_COMMIT}"
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(imkit_imgui_source)

    add_library(imkit_bundled_imgui STATIC
        "${imkit_imgui_source_SOURCE_DIR}/imgui.cpp"
        "${imkit_imgui_source_SOURCE_DIR}/imgui_demo.cpp"
        "${imkit_imgui_source_SOURCE_DIR}/imgui_draw.cpp"
        "${imkit_imgui_source_SOURCE_DIR}/imgui_tables.cpp"
        "${imkit_imgui_source_SOURCE_DIR}/imgui_widgets.cpp"
    )
    target_compile_features(imkit_bundled_imgui PUBLIC cxx_std_20)
    target_include_directories(imkit_bundled_imgui
        PUBLIC "${imkit_imgui_source_SOURCE_DIR}"
    )

    set(${out_target} imkit_bundled_imgui PARENT_SCOPE)
    set(${out_source_dir} "${imkit_imgui_source_SOURCE_DIR}" PARENT_SCOPE)
endfunction()

function(imkit_find_imgui_source_dir_from_target imgui_target out_source_dir)
    get_target_property(imgui_include_dirs "${imgui_target}" INTERFACE_INCLUDE_DIRECTORIES)
    if(NOT imgui_include_dirs)
        set(${out_source_dir} "" PARENT_SCOPE)
        return()
    endif()

    foreach(include_dir IN LISTS imgui_include_dirs)
        if(include_dir MATCHES "^\\$<BUILD_INTERFACE:(.*)>$")
            set(include_dir "${CMAKE_MATCH_1}")
        elseif(include_dir MATCHES "^\\$<")
            continue()
        endif()
        if(EXISTS "${include_dir}/backends/imgui_impl_glfw.cpp"
           AND EXISTS "${include_dir}/backends/imgui_impl_opengl3.cpp")
            set(${out_source_dir} "${include_dir}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    set(${out_source_dir} "" PARENT_SCOPE)
endfunction()

function(imkit_provide_gallery_dependencies imgui_target imgui_source_dir out_backend_target)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        imkit_glfw_source
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG "${IMKIT_GLFW_COMMIT}"
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(imkit_glfw_source)

    find_package(OpenGL REQUIRED)

    add_library(imkit_gallery_backends STATIC
        "${imgui_source_dir}/backends/imgui_impl_glfw.cpp"
        "${imgui_source_dir}/backends/imgui_impl_opengl3.cpp"
    )
    target_compile_features(imkit_gallery_backends PUBLIC cxx_std_20)
    target_include_directories(imkit_gallery_backends
        PUBLIC "${imgui_source_dir}/backends"
    )
    target_link_libraries(imkit_gallery_backends
        PUBLIC
            "${imgui_target}"
            glfw
            OpenGL::GL
    )

    set(${out_backend_target} imkit_gallery_backends PARENT_SCOPE)
endfunction()
