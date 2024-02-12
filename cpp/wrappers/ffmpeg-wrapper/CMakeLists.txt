project (ffmpeg-wrapper)

FILE( GLOB_RECURSE PUBLIC_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.h)
#FILE( GLOB_RECURSE PUBLIC_INL RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.inl)
FILE( GLOB_RECURSE SRC RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.cpp)

tool_define_source_group(
    ${LOCAL_PUBLIC_HEADERS}
    ${LOCAL_SRC}
)

find_package(InteractiveToolkit REQUIRED QUIET)

add_library( ${PROJECT_NAME} STATIC ${PUBLIC_HEADERS} ${SRC} )

# set the target's folder (for IDEs that support it, e.g. Visual Studio)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "LIBS")


target_compile_options(${PROJECT_NAME} 
PUBLIC
    ${INTERACTIVETOOLKIT_COMPILE_OPTIONS}
)

target_link_options(${PROJECT_NAME} 
PUBLIC
    ${INTERACTIVETOOLKIT_LINK_OPTIONS}
)

target_include_directories(${PROJECT_NAME} 
PUBLIC
    ${INTERACTIVETOOLKIT_INCLUDE_DIRS}
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)


#link libraries
#not used: libavdevice libavfilter libpostproc
target_link_libraries(${PROJECT_NAME} 
PUBLIC 
    ${INTERACTIVETOOLKIT_LIBRARIES}
    libavcodec libavformat libavutil libswresample libswscale
)

#copy_headers_to_include_directory(${PROJECT_NAME} ${PUBLIC_HEADERS})
#configure_include_file( ${PROJECT_NAME} "${PROJECT_NAME}.h.in" "${PROJECT_NAME}.h" ${PUBLIC_HEADERS} )

message(STATUS "")
message(STATUS "FFMPEG API CHANGES AT:")
message(STATUS "    https://github.com/FFmpeg/FFmpeg/blob/master/doc/APIchanges")
message(STATUS "")