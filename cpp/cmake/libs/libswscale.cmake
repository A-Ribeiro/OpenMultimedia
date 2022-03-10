if (NOT TARGET libswscale)
    message(STATUS "    Creating target: libswscale")
    add_library(libswscale OBJECT ${SWSCALE_LIBRARIES})
    target_link_libraries(libswscale ${SWSCALE_LIBRARIES})
    set_target_properties(libswscale PROPERTIES LINKER_LANGUAGE CXX)
    # set the target's folder (for IDEs that support it, e.g. Visual Studio)
    set_target_properties(libswscale PROPERTIES FOLDER "FFMPEG-LIBS")
    if (OS_TARGET STREQUAL win)
        ffmpeg_query_wildcard(${SWSCALE_LIBRARIES} swscale*.dll DLL_FILE)
        ffmpeg_copy_dll_after_build(libswscale ${DLL_FILE})
    endif()
    #include_directories(${SWSCALE_INCLUDE_DIR})
endif()