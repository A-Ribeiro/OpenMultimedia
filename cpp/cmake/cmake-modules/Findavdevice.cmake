if(AVDEVICE_INCLUDE_DIR AND AVDEVICE_LIBRARIES)
	unset(AVDEVICE_INCLUDE_DIR)
	unset(AVDEVICE_LIBRARIES)
endif()

if (AVDEVICE_ROOT)
	find_path(AVDEVICE_INCLUDE_DIR libavdevice/avdevice.h PATHS ${AVDEVICE_ROOT})
	find_library(AVDEVICE_LIBRARIES NAMES avdevice avdevice.58 PATHS ${AVDEVICE_ROOT})
else()
	find_path(AVDEVICE_INCLUDE_DIR libavdevice/avdevice.h)
	find_library(AVDEVICE_LIBRARIES NAMES avdevice avdevice.58)
endif()

if(AVDEVICE_INCLUDE_DIR AND AVDEVICE_LIBRARIES)
	set(AVDEVICE_FOUND ON)
endif()

if(AVDEVICE_FOUND)
	if (NOT ${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
		MESSAGE(STATUS "Found ${CMAKE_FIND_PACKAGE_NAME} include:  ${AVDEVICE_INCLUDE_DIR}")
		MESSAGE(STATUS "Found ${CMAKE_FIND_PACKAGE_NAME} library: ${AVDEVICE_LIBRARIES}")
	endif()
else()
	if(${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "Could NOT find ${CMAKE_FIND_PACKAGE_NAME} development files")
	endif()
endif()