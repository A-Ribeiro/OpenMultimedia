############################################################################
# Detect FFmpeg libs
############################################################################
message(STATUS "[FFmpeg Detector]" )

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake-modules)

get_filename_component(CMAKE_PARENT_LIST_DIR ${CMAKE_CURRENT_LIST_DIR}/.. ABSOLUTE)
#cmake_path(GET ${CMAKE_CURRENT_LIST_DIR} PARENT_PATH CMAKE_PARENT_LIST_DIR)

if (OS_TARGET STREQUAL win)
    set(FFMPEG_ROOT ${CMAKE_PARENT_LIST_DIR}/lgpl/ffmpeg/include )
    if( ARCH_TARGET STREQUAL x86 )
        list(APPEND FFMPEG_ROOT ${CMAKE_PARENT_LIST_DIR}/lgpl/ffmpeg/lib_w32 )
    elseif( ARCH_TARGET STREQUAL x64 )
        list(APPEND FFMPEG_ROOT ${CMAKE_PARENT_LIST_DIR}/lgpl/ffmpeg/lib_w64 )
    else()
        message(FATAL_ERROR "windows FFmpeg build arch not supported...")
    endif()
    set(AVCODEC_ROOT ${FFMPEG_ROOT})
    set(AVDEVICE_ROOT ${FFMPEG_ROOT})
    set(AVFILTER_ROOT ${FFMPEG_ROOT})
    set(AVFORMAT_ROOT ${FFMPEG_ROOT})
    set(AVUTIL_ROOT ${FFMPEG_ROOT})
    set(POSTPROC_ROOT ${FFMPEG_ROOT})
    set(SWRESAMPLE_ROOT ${FFMPEG_ROOT})
    set(SWSCALE_ROOT ${FFMPEG_ROOT})
elseif (OS_TARGET STREQUAL mac)

    if (ARCH_TARGET STREQUAL x64)
        set(FFMPEG_ROOT 
            ${CMAKE_PARENT_LIST_DIR}/lgpl/ffmpeg/include
            ${CMAKE_PARENT_LIST_DIR}/lgpl/ffmpeg/lib)
        set(AVCODEC_ROOT ${FFMPEG_ROOT})
        set(AVDEVICE_ROOT ${FFMPEG_ROOT})
        set(AVFILTER_ROOT ${FFMPEG_ROOT})
        set(AVFORMAT_ROOT ${FFMPEG_ROOT})
        set(AVUTIL_ROOT ${FFMPEG_ROOT})
        set(POSTPROC_ROOT ${FFMPEG_ROOT})
        set(SWRESAMPLE_ROOT ${FFMPEG_ROOT})
        set(SWSCALE_ROOT ${FFMPEG_ROOT})
    elseif (ARCH_TARGET STREQUAL arm64)
        message(FATAL_ERROR "FFMPEG not compiled for this platform.")
    endif()

endif()

find_package(avcodec REQUIRED QUIET)
find_package(avdevice REQUIRED QUIET)
find_package(avfilter REQUIRED QUIET)
find_package(avformat REQUIRED QUIET)
find_package(avutil REQUIRED QUIET)
find_package(postproc REQUIRED QUIET)
find_package(swresample REQUIRED QUIET)
find_package(swscale REQUIRED QUIET)

set(FFMPEG_INCLUDE_DIR
    ${AVCODEC_INCLUDE_DIR}
    ${AVDEVICE_INCLUDE_DIR}
    ${AVFILTER_INCLUDE_DIR}
    ${AVFORMAT_INCLUDE_DIR}
    ${AVUTIL_INCLUDE_DIR}
    ${POSTPROC_INCLUDE_DIR}
    ${SWRESAMPLE_INCLUDE_DIR}
    ${SWSCALE_INCLUDE_DIR})

list(REMOVE_DUPLICATES FFMPEG_INCLUDE_DIR)

macro(ffmpeg_query_wildcard LIBRARY_DIR PATTERN OUTPUT )
    get_filename_component(AUX ${LIBRARY_DIR} DIRECTORY)
    FILE( GLOB AUX ${AUX}/${PATTERN})
    list(GET AUX 0 AUX)
    set(${OUTPUT} ${AUX})
endmacro()

if (OS_TARGET STREQUAL win)
    macro(ffmpeg_copy_dll_after_build target dll_name)

        set(aux_project_name "${target}_copy_dll")

        configure_file(${CMAKE_CURRENT_LIST_DIR}/template.in ${CMAKE_CURRENT_BINARY_DIR}/${aux_project_name}.h)

        add_library(${aux_project_name} ${CMAKE_CURRENT_BINARY_DIR}/${aux_project_name}.h)
        set_target_properties(${aux_project_name} PROPERTIES LINKER_LANGUAGE CXX)
        # set the target's folder (for IDEs that support it, e.g. Visual Studio)
        set_target_properties(${aux_project_name} PROPERTIES FOLDER "FFMPEG-LIBS")
        copy_file_after_build(${aux_project_name} ${dll_name})

        add_dependencies(${target} ${aux_project_name})

    endmacro()
endif()

include_directories(${FFMPEG_INCLUDE_DIR})

include(${CMAKE_CURRENT_LIST_DIR}/libs/libavcodec.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/libs/libavdevice.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/libs/libavfilter.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/libs/libavformat.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/libs/libavutil.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/libs/libpostproc.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/libs/libswresample.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/libs/libswscale.cmake)

#message(STATUS "    FFMPEG_INCLUDE_DIR: ${FFMPEG_INCLUDE_DIR}" )

if (MSVC AND ARCH_TARGET STREQUAL x86)
    # disable safe exception handler on the compiler... 
    # it is for x86 arch on VS
    add_link_options(/SAFESEH:NO)
    #SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
endif()
