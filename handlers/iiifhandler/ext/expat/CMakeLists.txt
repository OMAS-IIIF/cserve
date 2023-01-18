cmake_minimum_required(VERSION 3.24.0)
project(expat_builder C CXX)
include(ExternalProject)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.24.0)
    set(timestamp_policy DOWNLOAD_EXTRACT_TIMESTAMP OLD)
else()
    unset(timestamp_policy)
endif()

ExternalProject_Add(project_expat
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://github.com/libexpat/libexpat/releases/download/R_2_4_8/expat-2.4.8.tar.bz2
        ${timestamp_policy}
        SOURCE_DIR ${COMMON_SRCS}/expat
        CONFIGURE_COMMAND ${COMMON_SRCS}/expat/configure
            --prefix=${COMMON_LOCAL}
            --libdir=${CONFIGURE_LIBDIR}
        BUILD_COMMAND make
        BUILD_IN_SOURCE 1
)

ExternalProject_Get_Property(project_expat install_dir)

if(MAKE_SHARED_SIPI)
    add_library(expat SHARED IMPORTED GLOBAL)
    set_property(TARGET expat PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libexpat${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(expat STATIC IMPORTED GLOBAL)
    set_property(TARGET expat PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libexpat${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

add_dependencies(expat project_expat)


