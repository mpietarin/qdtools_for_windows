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
