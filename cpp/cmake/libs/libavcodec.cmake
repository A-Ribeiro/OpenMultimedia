if (NOT TARGET libavcodec)
    message(STATUS "    Creating target: libavcodec")
    add_library(libavcodec OBJECT ${AVCODEC_LIBRARIES})
    target_link_libraries(libavcodec ${AVCODEC_LIBRARIES})
    set_target_properties(libavcodec PROPERTIES LINKER_LANGUAGE CXX)
    # set the target's folder (for IDEs that support it, e.g. Visual Studio)
    set_target_properties(libavcodec PROPERTIES FOLDER "FFMPEG-LIBS")
    if (OS_TARGET STREQUAL win)
        ffmpeg_query_wildcard(${AVCODEC_LIBRARIES} avcodec*.dll DLL_FILE)
        ffmpeg_copy_dll_after_build(libavcodec ${DLL_FILE})
    elseif (OS_TARGET STREQUAL mac)
        ffmpeg_query_wildcard(${AVCODEC_LIBRARIES} libavcodec*.dylib DYN_LIB)
        get_filename_component(DYN_LIB_NAME ${DYN_LIB} NAME)
        configure_file("${DYN_LIB}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${DYN_LIB_NAME}" COPYONLY)
    endif()
    #include_directories(${AVCODEC_INCLUDE_DIR})
endif()