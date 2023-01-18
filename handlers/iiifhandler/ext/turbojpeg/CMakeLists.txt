cmake_minimum_required(VERSION 3.14.5)
project(jpeg_builder C CXX)
include(ExternalProject)

#
# get libjpeg-turbo
#
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.24.0)
    set(timestamp_policy DOWNLOAD_EXTRACT_TIMESTAMP OLD)
else()
    unset(timestamp_policy)
endif()

ExternalProject_Add(project_turbojpeg
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://github.com/libjpeg-turbo/libjpeg-turbo/archive/refs/tags/2.0.7-esr.tar.gz
        ${timestamp_policy}
        SOURCE_DIR ${COMMON_SRCS}/libjpeg-turbo
        CMAKE_ARGS -DCMAKE_PREFIX_PATH=${COMMON_LOCAL}
        -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL}
        -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
    BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(project_turbojpeg install_dir)

if(MAKE_SHARED_CSERVE)
    add_library(turbojpeg SHARED IMPORTED GLOBAL)
    set_property(TARGET turbojpeg PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libturbojpeg${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(CMAKE_CREATE_SHARED "-DBUILD_SHARED_LIBS=ON")
else()
    add_library(turbojpeg STATIC IMPORTED GLOBAL)
    set_property(TARGET turbojpeg PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libturbojpeg${CMAKE_STATIC_LIBRARY_SUFFIX})
    set(CMAKE_CREATE_SHARED "-DBUILD_SHARED_LIBS=OFF")
endif()

add_dependencies(turbojpeg project_turbojpeg)
