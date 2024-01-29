cmake_minimum_required(VERSION 3.14.4)
project(qdtools)

if(WIN32)
    set(CMAKE_MFC_FLAG 0)
endif()

file( GLOB SOURCE_FILES source/* )

add_library( 
    ${PROJECT_NAME} STATIC
    ${SOURCE_FILES} 
    )

target_include_directories( 
    ${PROJECT_NAME}
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    PUBLIC ${Newbase_INCLUDE_DIRS}
    PUBLIC ${Imagine_INCLUDE_DIRS}
    PUBLIC ${BOOST_INCLUDE_DIR} 
    PUBLIC ${FMT_INCLUDE_DIR}
    PUBLIC ${GDAL_INCLUDE_DIR} 
    )

set(
    ${PROJECT_NAME}_INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}/source
    ${PROJECT_SOURCE_DIR}
    ${Newbase_INCLUDE_DIRS}
    ${Imagine_INCLUDE_DIRS}
    ${BOOST_INCLUDE_DIR} 
    ${FMT_INCLUDE_DIR}
    ${GDAL_INCLUDE_DIR} 
    CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE
    )

set(
	INCLUDE_DIRECTORY
	${PROJECT_SOURCE_DIR}/include
)

set(
	SOURCE_DIRECTORY
	${PROJECT_SOURCE_DIR}/source
)

set(
	MAIN_DIRECTORY
	${PROJECT_SOURCE_DIR}/main
)

add_subdirectory(
	main-cmake-lists/flash2qd
)

add_subdirectory(
	main-cmake-lists/qdclimatologydatasmoother
)

add_subdirectory(
	main-cmake-lists/qdcrop
)

add_subdirectory(
	main-cmake-lists/qddifference
)

add_subdirectory(
	main-cmake-lists/qdsignificantlevelfilter
)

add_subdirectory(
	main-cmake-lists/qdsoundingindex
)

add_subdirectory(
	main-cmake-lists/qdinterpolatetime
)

add_subdirectory(
	main-cmake-lists/qddiff
)

add_subdirectory(
	main-cmake-lists/qdstat
)

add_subdirectory(
	main-cmake-lists/qdscript
)

add_subdirectory(
	main-cmake-lists/grd2qd
)

add_subdirectory(
	main-cmake-lists/qdversionchange
)

add_subdirectory(
	main-cmake-lists/gribtoqd
)
