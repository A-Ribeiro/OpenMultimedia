project (ipc-syphon-source)

#set(CMAKE_BUILD_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/Syphon5")
#set(CMAKE_MACOSX_RPATH 1)


include("${CMAKE_HOME_DIRECTORY}/cpp/libs/InteractiveToolkit-Extension/cmake/libLinkOpenGL.cmake")

FILE( GLOB_RECURSE PUBLIC_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" src/*.h)
FILE( GLOB_RECURSE SRC RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" src/*.mm)

#include_directories( "${CMAKE_CURRENT_LIST_DIR}/Syphon5/Syphon.framework/Headers" )
FILE( GLOB_RECURSE SYPHON_HEADERS "${CMAKE_CURRENT_LIST_DIR}/Syphon5/Syphon.framework/Headers/*.h")
#copy_headers_to_include_directory(Syphon ${SYPHON_HEADERS})

add_executable(${PROJECT_NAME} ${PUBLIC_HEADERS} ${SRC})

target_compile_options(${PROJECT_NAME} PUBLIC -DGL_SILENCE_DEPRECATION)

tool_define_source_group_base_path("src/" ${PUBLIC_HEADERS} ${SRC})

# set the target's folder (for IDEs that support it, e.g. Visual Studio)
#set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "TOOLS")
#find_library(SYPHON_LIB Syphon "${CMAKE_CURRENT_LIST_DIR}/Syphon5/")

target_link_libraries(${PROJECT_NAME} 
    InteractiveToolkit-Extension
    #-framework Syphon
    #${SYPHON_LIB}
    "${CMAKE_CURRENT_LIST_DIR}/Syphon5/Syphon.framework"
    "-framework Foundation"
    LinkOpenGL
)

file(COPY "${CMAKE_CURRENT_LIST_DIR}/Syphon5/Syphon.framework"
     DESTINATION "${CMAKE_BINARY_DIR}/Frameworks/")
