cmake_minimum_required (VERSION 3.5.1)

project(OpenMultimedia)

# "For this is how God loved the world:
# he gave his only Son, so that everyone
# who believes in him may not perish
# but may have eternal life."
#
# John 3:16

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

ENABLE_LANGUAGE(CXX)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# avoid base libs overwrite
option(ARIBEIRO_SKIP_INSTALL_CORE ON)
option(ARIBEIRO_SKIP_INSTALL_PLATFORM ON)
option(ARIBEIRO_SKIP_INSTALL_DATA ON)

if(MSVC)
    #disable size_t warning conversion... on 64build
    add_compile_options( /wd4267 )
endif()

include(cmake/include.cmake)

# we need to include this first to be available to use in our projects.
# this lib is used in the aRibeiroData also.
include(cmake/libs/libzlib.cmake)

include(cmake/libs/libaRibeiroCore.cmake)
include(cmake/libs/libaRibeiroPlatform.cmake)
include(cmake/libs/libaRibeiroData.cmake)

#include_directories(${ARIBEIRO_GEN_INCLUDE_DIR}/zlib/)

#include(cmake/libs/libLinkOpenGL.cmake)

#include(cmake/libs/aRibeiroWrappers.cmake)
#tool_include_lib(aRibeiroWrappers ft2-wrapper)

#add_subdirectory(src/glad)
#add_subdirectory(src/opengl-wrapper)
#add_subdirectory(src/window-gl-wrapper)

include(cpp/cmake/detect_ffmpeg.cmake)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


add_subdirectory(cpp/wrappers/ffmpeg-wrapper)

add_subdirectory(cpp/src/debug_console)
add_subdirectory(cpp/src/network_to_ipc)

if(MSVC)
    set(cmake_license_file_abs_path "${CMAKE_CURRENT_LIST_DIR}/LICENSE")
    configure_file(
        ${CMAKE_CURRENT_LIST_DIR}/inno_setup_install_generator.iss.in
        "${CMAKE_BINARY_DIR}/inno_setup_install_generator.iss"
        @ONLY
    )
    add_subdirectory(cpp/src/ipc_dshow_source)
else()
    message(STATUS "NOTE: to compile 'ipc_dshow_source' you need to set the Visual Studio IDE")
endif()

#add_subdirectory(src/mini-gl-engine)

#add_subdirectory(src/tech-demo-pbr)
#add_subdirectory(src/tech-demo-particle)
#add_subdirectory(src/tech-demo-animation)

