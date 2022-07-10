cmake_minimum_required(VERSION 3.14.5)
project(jpeg_builder C CXX)
include(ExternalProject)

#
# get libjpeg 1.4.1
#
ExternalProject_Add(project_jpeg
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://github.com/libjpeg-turbo/libjpeg-turbo/archive/refs/tags/2.0.7-esr.tar.gz
        SOURCE_DIR ${COMMON_SRCS}/libjpeg-turbo
        CMAKE_ARGS -DCMAKE_PREFIX_PATH=${COMMON_LOCAL}
        -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL}
        -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
    BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(project_jpeg install_dir)

if(MAKE_SHARED_CSERVE)
    add_library(jpeg SHARED IMPORTED GLOBAL)
    set_property(TARGET jpeg PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libjpeg${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(CMAKE_CREATE_SHARED "-DBUILD_SHARED_LIBS=ON")
else()
    add_library(jpeg STATIC IMPORTED GLOBAL)
    set_property(TARGET jpeg PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libjpeg${CMAKE_STATIC_LIBRARY_SUFFIX})
    set(CMAKE_CREATE_SHARED "-DBUILD_SHARED_LIBS=OFF")
endif()

add_dependencies(jpeg project_jpeg)
