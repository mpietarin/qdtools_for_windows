
set(
    LIBS_3RD
    D:/projekti/ver200_SmartMet_release_5_12/qdtools_for_windows/modules/3rd
    CACHE INTERNAL "" FORCE
    )

set(
    BOOST_VERSION
    1_61_0
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

set(
    ICU_VERSION
    55_1
    CACHE INTERNAL "" FORCE
    )

set(
    ICU_BASE_DIR
    ${LIBS_3RD}/icu_${ICU_VERSION}
    CACHE INTERNAL "" FORCE
    )

set(
    ICU_LIB_DIR
    ${ICU_BASE_DIR}/lib
    CACHE INTERNAL "" FORCE
    )
