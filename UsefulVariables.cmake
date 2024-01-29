set(
    ROOT 
    ${CMAKE_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
    )

set(
    LIBS_3RD
    ${ROOT}/modules/3rd
    CACHE INTERNAL "" FORCE
    )

# **** Boost library section ****
set(
    BOOST_VERSION
    1_70_0
    CACHE INTERNAL "" FORCE
    )

set(
    BOOST_BASE_DIR
    ${LIBS_3RD}/boost_${BOOST_VERSION}
    CACHE INTERNAL "" FORCE
    )

set(
    BOOST_INCLUDE_DIR
    ${BOOST_BASE_DIR}
    CACHE INTERNAL "" FORCE
    )

set(
    BOOST_LIB_DIR
    ${BOOST_BASE_DIR}/lib
    CACHE INTERNAL "" FORCE
    )

# **** Fmt library section ****
set(
    FMT_VERSION
    7_0_3
    CACHE INTERNAL "" FORCE
    )

set(
    FMT_BASE_DIR
    ${LIBS_3RD}/fmt_${FMT_VERSION}
    CACHE INTERNAL "" FORCE
    )

set(
    FMT_INCLUDE_DIR
    ${FMT_BASE_DIR}/inc
    CACHE INTERNAL "" FORCE
    )

set(
    FMT_LIB_DIR
    ${FMT_BASE_DIR}/lib
    CACHE INTERNAL "" FORCE
    )

set(
    FMT_LIB_DEBUG
    ${FMT_LIB_DIR}/fmtd.lib
    CACHE INTERNAL "" FORCE
    )

set(
    FMT_LIB_RELEASE
    ${FMT_LIB_DIR}/fmt.lib
    CACHE INTERNAL "" FORCE
    )

set(
    FMT_TARGET_LINK_LIBRARIES
    debug ${FMT_LIB_DEBUG}
    optimized ${FMT_LIB_RELEASE}
    )

# Disable VC++ warning in Linux based modules with CMake macro. 
# Because compilers (MSVC++ and gcc) use different definitions for different types.
# Unable to prevent these the correct way, because not developing these modules and changes would mess up linux side productions.

macro(disable_linux_side_compiler_warnings)

# Warning C4068 about: unknown pragma 'clang'
add_compile_options(
    $<$<CXX_COMPILER_ID:MSVC>:/wd4068>
    )

# Warning C4099 about: 'TextGen::FogIntensityDataItem': type name first seen using 'class' now seen using 'struct'
add_compile_options(
    $<$<CXX_COMPILER_ID:MSVC>:/wd4099>
    )

# Warning C4146 about: unary minus operator applied to unsigned type, result still unsigned
add_compile_options(
    $<$<CXX_COMPILER_ID:MSVC>:/wd4146>
    )

# Warning C4244 about: 'return': conversion from 'double' to 'float
add_compile_options(
    $<$<CXX_COMPILER_ID:MSVC>:/wd4244>
    )

# Warning C4267 about: 'initializing': conversion from 'size_t' to 'unsigned int'
add_compile_options(
    $<$<CXX_COMPILER_ID:MSVC>:/wd4267>
    )

# Warning C4305 about: 'argument': truncation from 'double' to 'float'
add_compile_options(
    $<$<CXX_COMPILER_ID:MSVC>:/wd4305>
    )

# Warning C4333 about: '>>': right shift by too large amount, data loss
add_compile_options(
    $<$<CXX_COMPILER_ID:MSVC>:/wd4333>
    )

# Warning C4804 about: '>': unsafe use of type 'bool' in operation
add_compile_options(
    $<$<CXX_COMPILER_ID:MSVC>:/wd4804>
    )

# Warning C4805 about: '|': unsafe mix of type 'int' and type 'bool' in operation
add_compile_options(
    $<$<CXX_COMPILER_ID:MSVC>:/wd4805>
    )

# Warning C4834 about: discarding return value of function with 'nodiscard' attribute
add_compile_options(
    $<$<CXX_COMPILER_ID:MSVC>:/wd4834>
    )

endmacro()
