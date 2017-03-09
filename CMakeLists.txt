cmake_minimum_required(VERSION 3.7.1)

project(qdtools_for_windows)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/runtime_output")

add_definitions(
    -D_WIN64
    -D_UNICODE 
    -DUNICODE 
    )

add_compile_options(
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
    )

include(
    UsefulVariables.cmake
    )

set(UPDATE_DISCONNECTED_IF_AVAILABLE "UPDATE_DISCONNECTED 1")

include(DownloadProject.cmake)
download_project(PROJ                googletest
    GIT_REPOSITORY      https://github.com/google/googletest.git
    GIT_TAG             master
    ${UPDATE_DISCONNECTED_IF_AVAILABLE}
    )

# Prevent GoogleTest from overriding our compiler/linker options
# when building with Visual Studio
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(
    ${googletest_SOURCE_DIR}
    ${googletest_BINARY_DIR}
    )

add_subdirectory(
    modules/newbase
    )

add_subdirectory(
    modules/smarttools
    )

add_subdirectory(
    qdtools
    )
