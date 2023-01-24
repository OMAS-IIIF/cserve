cmake_minimum_required(VERSION 3.14.5)
project(tiff_builder C CXX)
include(ExternalProject)

if(MAKE_SHARED_CSERVE)
    SET(TIFF_LIBRARY "libtiff${CMAKE_SHARED_LIBRARY_SUFFIX}")
    SET(TIFFXX_LIBRARY "libtiffxx${CMAKE_SHARED_LIBRARY_SUFFIX}")
else()
    SET(TIFF_LIBRARY "libtiff${CMAKE_STATIC_LIBRARY_SUFFIX}")
    SET(TIFFXX_LIBRARY "libtiffxx${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(CMAKE_CREATE_SHARED "-DBUILD_SHARED_LIBS=OFF")
endif()

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.24.0)
    set(timestamp_policy DOWNLOAD_EXTRACT_TIMESTAMP OLD)
else()
    unset(timestamp_policy)
endif()

ExternalProject_Add(project_tiff
        DEPENDS xz
        DEPENDS zlib
        DEPENDS deflate
        DEPENDS lerc
        DEPENDS turbojpeg
        DEPENDS webp
        DEPENDS lerc
        DEPENDS zstd
        DEPENDS jbigkit
        INSTALL_DIR ${COMMON_LOCAL}
        SOURCE_DIR ${COMMON_SRCS}/libtiff-4.5.0
        URL http://download.osgeo.org/libtiff/tiff-4.5.0.tar.xz
        ${timestamp_policy}
        DOWNLOAD_DIR ${COMMON_SRCS}/downloads
        CMAKE_ARGS  -Dtiff-tools:BOOL=OFF
                    -Dtiff-tests:BOOL=OFF
                    -Dtiff-contrib:BOOL=OFF
                    -Dtiff-docs:BOOL=OFF
                    ${CMAKE_CREATE_SHARED}
                    -DCMAKE_PREFIX_PATH=${COMMON_LOCAL}
                    -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL}
                    -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
        BUILD_IN_SOURCE 1
)

ExternalProject_Add_Step(project_tiff gaga
        DEPENDEES install
        COMMENT "Copy tif_dir.h.."
        COMMAND ${CMAKE_COMMAND} -E copy ${COMMON_SRCS}/libtiff-4.5.0/libtiff/tif_dir.h ${COMMON_LOCAL}/include/)

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
