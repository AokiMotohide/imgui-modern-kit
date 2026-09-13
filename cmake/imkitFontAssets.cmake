function(imkit_copy_font_assets target destination)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "imkit_copy_font_assets target does not exist: ${target}")
    endif()
    if(NOT DEFINED IMKIT_FONT_ASSET_DIR OR NOT IS_DIRECTORY "${IMKIT_FONT_ASSET_DIR}")
        message(FATAL_ERROR "ImKit font asset directory is unavailable: ${IMKIT_FONT_ASSET_DIR}")
    endif()
    if(APPLE)
        set(asset_destination "$<TARGET_BUNDLE_DIR:${target}>/Contents/Resources/${destination}")
    else()
        set(asset_destination "$<TARGET_FILE_DIR:${target}>/${destination}")
    endif()
    add_custom_command(TARGET "${target}" POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${asset_destination}"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${IMKIT_FONT_ASSET_DIR}"
            "${asset_destination}"
        VERBATIM)
endfunction()
