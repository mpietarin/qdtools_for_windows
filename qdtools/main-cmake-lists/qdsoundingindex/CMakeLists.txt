cmake_minimum_required(VERSION 3.14.4)
project(qdsoundingindex)

file( 
GLOB CPP_FILES 
${MAIN_DIRECTORY}/qdsoundingindex.cpp
 )

link_directories(
    ${BOOST_LIB_DIR}
)

add_executable( 
    ${PROJECT_NAME}
    ${CPP_FILES} 
    )

target_link_libraries(
    ${PROJECT_NAME}
    Newbase
    SmartTools
    ${FMT_TARGET_LINK_LIBRARIES}
    )

target_include_directories( 
    ${PROJECT_NAME}
    PUBLIC ${Newbase_INCLUDE_DIRS}
    PUBLIC ${SmartTools_INCLUDE_DIRS}
    PUBLIC ${BOOST_INCLUDE_DIR}
    )
