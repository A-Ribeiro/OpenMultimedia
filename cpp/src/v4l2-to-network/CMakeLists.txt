project (v4l2-to-network)

FILE( GLOB_RECURSE PUBLIC_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.h)
FILE( GLOB_RECURSE SRC RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.cpp)

add_executable(${PROJECT_NAME} ${PUBLIC_HEADERS} ${SRC})

tool_define_source_group(${PUBLIC_HEADERS} ${SRC})

target_link_libraries(${PROJECT_NAME} 
    InteractiveToolkit
)
