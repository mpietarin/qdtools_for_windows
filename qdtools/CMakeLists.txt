cmake_minimum_required(VERSION 3.14.4)
project(qdtools)

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
