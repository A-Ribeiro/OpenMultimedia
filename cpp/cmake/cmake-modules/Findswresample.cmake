if(SWRESAMPLE_INCLUDE_DIR AND SWRESAMPLE_LIBRARIES)
	unset(SWRESAMPLE_INCLUDE_DIR)
	unset(SWRESAMPLE_LIBRARIES)
endif()

if (SWRESAMPLE_ROOT)
	find_path(SWRESAMPLE_INCLUDE_DIR libswresample/swresample.h PATHS ${SWRESAMPLE_ROOT})
	find_library(SWRESAMPLE_LIBRARIES NAMES swresample swresample.3 PATHS ${SWRESAMPLE_ROOT})
else()
	find_path(SWRESAMPLE_INCLUDE_DIR libswresample/swresample.h)
	find_library(SWRESAMPLE_LIBRARIES NAMES swresample swresample.3)
endif()

if(SWRESAMPLE_INCLUDE_DIR AND SWRESAMPLE_LIBRARIES)
	set(SWRESAMPLE_FOUND ON)
endif()

if(SWRESAMPLE_FOUND)
	if (NOT ${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
		MESSAGE(STATUS "Found ${CMAKE_FIND_PACKAGE_NAME} include:  ${SWRESAMPLE_INCLUDE_DIR}")
		MESSAGE(STATUS "Found ${CMAKE_FIND_PACKAGE_NAME} library: ${SWRESAMPLE_LIBRARIES}")
	endif()
else()
	if(${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "Could NOT find ${CMAKE_FIND_PACKAGE_NAME} development files")
	endif()
endif()