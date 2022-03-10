if (NOT TARGET libpostproc)
    message(STATUS "    Creating target: libpostproc")
    add_library(libpostproc OBJECT ${POSTPROC_LIBRARIES})
    target_link_libraries(libpostproc ${POSTPROC_LIBRARIES})
    set_target_properties(libpostproc PROPERTIES LINKER_LANGUAGE CXX)
    # set the target's folder (for IDEs that support it, e.g. Visual Studio)
    set_target_properties(libpostproc PROPERTIES FOLDER "FFMPEG-LIBS")
    if (OS_TARGET STREQUAL win)
        ffmpeg_query_wildcard(${POSTPROC_LIBRARIES} postproc*.dll DLL_FILE)
        ffmpeg_copy_dll_after_build(libpostproc ${DLL_FILE})
    endif()
    #include_directories(${POSTPROC_INCLUDE_DIR})
endif()