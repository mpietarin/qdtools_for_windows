get_filename_component(INSTALLED_PREFIX ${Newbase_DIR} DIRECTORY)
get_filename_component(INSTALLED_PREFIX ${INSTALLED_PREFIX} DIRECTORY)
get_filename_component(INSTALLED_PREFIX ${INSTALLED_PREFIX} DIRECTORY)
set(Newbase_INCLUDE_DIRS ${INSTALLED_PREFIX}/include)
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # If static and shared library is available, one can select
    # which one to use. By default use shared one.
    if(NOT ${PROJECT_NAME} MATCHES "Newbase")
        option(Newbase_USE_STATIC_LIBS "Newbase use static libs" OFF)
    endif()

    if(Newbase_USE_STATIC_LIBS)
        find_library (Newbase_STATIC_LIB "libNewbase.a" ${INSTALLED_PREFIX})
        set(Newbase_LIBRARIES  ${Newbase_STATIC_LIB})
    else()
        find_library (Newbase_SHARED_LIB "libNewbase.so" ${INSTALLED_PREFIX})
        set(Newbase_LIBRARIES  ${Newbase_SHARED_LIB})
    endif()
else()
    find_library (Newbase_STATIC_LIB "Newbase.lib" ${INSTALLED_PREFIX})
    set(Newbase_LIBRARIES  ${Newbase_STATIC_LIB})
endif()