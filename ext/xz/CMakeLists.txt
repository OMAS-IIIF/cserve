cmake_minimum_required(VERSION 3.14.5)
project(libxz_builder C CXX)
include(ExternalProject)

#
# libxyz
#
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.24.0)
    set(timestamp_policy DOWNLOAD_EXTRACT_TIMESTAMP OLD)
else ()
    unset(timestamp_policy)
endif ()

ExternalProject_Add(project_xz
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://github.com/tukaani-project/xz/releases/download/v5.4.6/xz-5.4.6.tar.gz
        ${timestamp_policy}
        SOURCE_DIR ${COMMON_SRCS}/xz-5.4.6
        CONFIGURE_COMMAND ${COMMON_SRCS}/xz-5.4.6/configure --prefix=${COMMON_LOCAL} --libdir=${CONFIGURE_LIBDIR}
        BUILD_COMMAND make
        BUILD_IN_SOURCE 1
        )
ExternalProject_Get_Property(project_xz install_dir)

if (MAKE_SHARED_CSERVE)
    add_library(xz SHARED IMPORTED GLOBAL)
    set_property(TARGET xz PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/liblzma${CMAKE_SHARED_LIBRARY_SUFFIX})
else ()
    add_library(xz STATIC IMPORTED GLOBAL)
    set_property(TARGET xz PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/liblzma${CMAKE_STATIC_LIBRARY_SUFFIX})
endif ()

add_dependencies(xz project_xz)
