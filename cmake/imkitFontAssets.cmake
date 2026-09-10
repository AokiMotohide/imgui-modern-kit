function(imkit_copy_font_assets target destination)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "imkit_copy_font_assets target does not exist: ${target}")
    endif()
    if(NOT DEFINED IMKIT_FONT_ASSET_DIR OR NOT IS_DIRECTORY "${IMKIT_FONT_ASSET_DIR}")
        message(FATAL_ERROR "ImKit font asset directory is unavailable: ${IMKIT_FONT_ASSET_DIR}")
    endif()
    add_custom_command(TARGET "${target}" POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_FILE_DIR:${target}>/${destination}"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${IMKIT_FONT_ASSET_DIR}"
            "$<TARGET_FILE_DIR:${target}>/${destination}"
        VERBATIM)
endfunction()
