cmake_minimum_required(VERSION 3.14.5)
project(tiff_builder C CXX)
include(ExternalProject)

#
# get libtiff-4.0.6
#

ExternalProject_Add(project_tiff
        DEPENDS xz
        DEPENDS jpeg
        DEPENDS jbigkit
        DEPENDS webp
        DEPENDS lerc
        INSTALL_DIR ${COMMON_LOCAL}
        TEST_BEFORE_INSTALL 1
        URL http://download.osgeo.org/libtiff/tiff-4.4.0.tar.xz
        SOURCE_DIR ${COMMON_SRCS}/libtiff-4.4.0
        CONFIGURE_COMMAND   ${COMMON_SRCS}/libtiff-4.4.0/configure
                            --prefix=${COMMON_LOCAL} --enable-shared --enable-static --libdir=${CONFIGURE_LIBDIR} --enable-cxx
        BUILD_COMMAND make
        INSTALL_COMMAND make test
        COMMAND make install
        COMMAND ${CMAKE_COMMAND} -E copy ${COMMON_SRCS}/libtiff-latest/libtiff/tif_dir.h ${COMMON_LOCAL}/include/
        BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(project_tiff install_dir)
if(MAKE_SHARED_CSERVE)
    add_library(tiff SHARED IMPORTED GLOBAL)
    set_property(TARGET tiff PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libtiff${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(CMAKE_CREATE_SHARED "-DBUILD_SHARED_LIBS=ON")
else()
    add_library(tiff STATIC IMPORTED GLOBAL)
    set_property(TARGET tiff PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libtiff${CMAKE_STATIC_LIBRARY_SUFFIX})
    set(CMAKE_CREATE_SHARED "-DBUILD_SHARED_LIBS=OFF")
endif()
add_dependencies(tiff project_tiff)
