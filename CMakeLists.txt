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

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# reset global variables
unset (CMAKE_MODULE_PATH CACHE)
unset (CMAKE_PREFIX_PATH CACHE)

# create CMAKE_BUILD_TYPE for non-windows platform
if( NOT CMAKE_BUILD_TYPE AND (NOT WIN32 OR CMAKE_COMPILER_IS_GNUCXX) )
    set( CMAKE_BUILD_TYPE Release CACHE STRING
            "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
            FORCE )
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS None Debug Release RelWithDebInfo MinSizeRel)
endif()

# configure compile vars
if(WIN32)
    #
    # Windows
    #
    if(CMAKE_COMPILER_IS_GNUCXX)

        if (CMAKE_BUILD_TYPE STREQUAL Release)
            add_definitions(-DNDEBUG)
            add_compile_options(-O3 -fno-strict-aliasing)
        endif()

    elseif(MSVC)
        # change the multithread debug DLL to multithread debug static compilation
        # avoid link with another VS DLL...
        foreach(flag
                CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO

                CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
                CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO )
            if(${flag} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
                SET ( ${flag} "${${flag}}" CACHE STRING "" FORCE)
            endif()
            set("${flag}" "${${flag}} /MP")
        endforeach()

        #SET(CMAKE_CXX_FLAGS_RELEASE  "${CMAKE_CXX_FLAGS_RELEASE} /O2")
        #SET(CMAKE_C_FLAGS_RELEASE  "${CMAKE_C_FLAGS_RELEASE} /O2")
    endif()
elseif(APPLE)
    #
    # MacOS X, iOS, watchOS, tvOS (since 3.10.3)
    #
    if (CMAKE_BUILD_TYPE STREQUAL Release)
        add_definitions(-DNDEBUG)
        add_compile_options(-O3 -fno-strict-aliasing)
    endif()
    #SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -all_load")
    #SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -v")
    #SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -ObjC")
elseif(UNIX)
    #
    # Linux, BSD, Solaris, Minix
    #
    if (CMAKE_BUILD_TYPE STREQUAL Release)
        add_definitions(-DNDEBUG)
        add_compile_options(-O3 -fno-strict-aliasing)
    elseif( CMAKE_BUILD_TYPE STREQUAL Debug)
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -static-libasan")
    endif()
else()
    message(FATAL_ERROR "Cmake not configured for this OS")
endif()

if(MSVC)
    #disable size_t warning conversion... on 64build
    add_compile_options( /wd4267 )
endif()


add_subdirectory(cpp/libs/InteractiveToolkit "${CMAKE_BINARY_DIR}/lib/InteractiveToolkit")

find_package(InteractiveToolkit REQUIRED)

if (NOT TARGET InteractiveToolkit-Extension)
    add_subdirectory(cpp/libs/InteractiveToolkit-Extension "${CMAKE_BINARY_DIR}/lib/InteractiveToolkit-Extension")
endif()

include(cpp/cmake/detect_ffmpeg.cmake)

add_subdirectory(cpp/wrappers/ffmpeg-wrapper)

add_subdirectory(cpp/src/debug-console)
add_subdirectory(cpp/src/network-to-ipc)

if(MSVC)
    set(cmake_license_file_abs_path "${CMAKE_CURRENT_LIST_DIR}/LICENSE")
    configure_file(
        ${CMAKE_CURRENT_LIST_DIR}/inno_setup_install_generator.iss.in
        "${CMAKE_BINARY_DIR}/inno_setup_install_generator.iss"
        @ONLY
    )
    add_subdirectory(cpp/src/ipc-dshow-source)
else()
    message(STATUS "NOTE: to compile 'ipc_dshow_source' you need to set the Visual Studio IDE")
endif()

if (APPLE)
    add_subdirectory(cpp/src/ipc-syphon-source)
else()
    message(STATUS "NOTE: to compile 'ipc_syphon_source' you need a Mac/x64 hardware")
endif()

if (UNIX)
    add_subdirectory(cpp/src/ipc-v4l2loopback-source)
    add_subdirectory(cpp/src/v4l2-to-network)
else()
    message(STATUS "NOTE: to compile 'ipc_v4l2loopback_source' you need a Linux OS")
endif()


if ((UNIX) OR (APPLE))
    add_subdirectory(cpp/src/clear-shm)
endif()

