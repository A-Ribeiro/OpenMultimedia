project (ipc-v4l2loopback-source)

FILE( GLOB_RECURSE PUBLIC_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.h)
FILE( GLOB_RECURSE SRC RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.cpp)

add_executable(${PROJECT_NAME} ${PUBLIC_HEADERS} ${SRC})

define_source_group(${PUBLIC_HEADERS} ${SRC})

target_link_libraries(${PROJECT_NAME} 
    aRibeiroCore aRibeiroPlatform aRibeiroData
)
