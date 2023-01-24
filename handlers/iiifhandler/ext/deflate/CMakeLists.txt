cmake_minimum_required(VERSION 3.14.5)
project(deflate_builder C CXX)
include(ExternalProject)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.24.0)
    set(timestamp_policy DOWNLOAD_EXTRACT_TIMESTAMP OLD)
else()
    unset(timestamp_policy)
endif()


ExternalProject_Add(
        project_deflate
        INSTALL_DIR ${COMMON_LOCAL}
        GIT_REPOSITORY https://github.com/ebiggers/libdeflate.git
        SOURCE_DIR ${COMMON_SRCS}/deflate
        CMAKE_ARGS  -DBUILD_SHARED_LIBS:BOOL=${MAKE_SHARED_CSERVE}
                    -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL}
                    -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
        BUILD_IN_SOURCE 1)

if(MAKE_SHARED_CSERVE)
    add_library(deflate SHARED IMPORTED GLOBAL)
    set_property(TARGET deflate PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libdeflate${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(deflate STATIC IMPORTED GLOBAL)
    set_property(TARGET deflate PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libdeflate${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

add_dependencies(deflate project_deflate)


