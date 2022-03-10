project (network-to-ipc)

FILE( GLOB_RECURSE PUBLIC_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.h)
FILE( GLOB_RECURSE SRC RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.cpp)

add_executable(${PROJECT_NAME} ${PUBLIC_HEADERS} ${SRC})

define_source_group(${PUBLIC_HEADERS} ${SRC})

# set the target's folder (for IDEs that support it, e.g. Visual Studio)
#set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "TOOLS")

# get_directory_property( inc_dir INCLUDE_DIRECTORIES )
# message(${inc_dir})

target_link_libraries(${PROJECT_NAME} 
    ffmpeg-wrapper
    aRibeiroCore aRibeiroPlatform aRibeiroData
    zlib
)

copy_file_after_build(${PROJECT_NAME} 
    resources/background_no_connection.png
    resources/network_to_ipc_ip.bat
)
