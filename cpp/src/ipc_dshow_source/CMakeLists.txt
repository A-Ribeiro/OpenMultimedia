project (ipc-dshow-source)

FILE( GLOB_RECURSE PUBLIC_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.h)
FILE( GLOB_RECURSE SRC RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.cpp)

include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/dshowdeps/include" )
LINK_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/dshowdeps/lib/${ARCH_TARGET}/" )

add_library(${PROJECT_NAME} SHARED ${PUBLIC_HEADERS} ${SRC} ipc_dshow_source.def ipc_dshow_source.rc splash.bin)

define_source_group(${PUBLIC_HEADERS} ${SRC} ipc_dshow_source.def ipc_dshow_source.rc splash.bin)

# set the target's folder (for IDEs that support it, e.g. Visual Studio)
#set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "TOOLS")

target_link_libraries(${PROJECT_NAME} 
    aRibeiroCore aRibeiroPlatform aRibeiroData
    strmiids
    # "${CMAKE_CURRENT_SOURCE_DIR}/dshowdeps/lib/${ARCH_TARGET}/strmbase.lib"
)

if(MSVC)
    add_link_options(/DEF:ipc_dshow_source.def /NODEFAULTLIB)
	#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEF:ipc_dshow_source.def /NODEFAULTLIB")
endif()

copy_file_after_build(${PROJECT_NAME} 
    resources/register_dll.ps1
    resources/unregister_dll.ps1

    "${CMAKE_BINARY_DIR}/inno_setup_install_generator.iss"
)