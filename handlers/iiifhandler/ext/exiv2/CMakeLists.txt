cmake_minimum_required(VERSION 3.14.5)
project(exiv2_builder C CXX)
include(ExternalProject)

include_directories(
        ${COMMON_LOCAL}/include
        /usr/local/include
        /usr/include
)

#
# get exiv2
#
#if(MAKE_SHARED_SIPI)
#    set(XMPDSK_LIBFILE "libxmpsdk${CMAKE_STATIC_LIBRARY_SUFFIX}")
#else()
#    set(XMPDSK_LIBFILE "libxmpsdk${CMAKE_STATIC_LIBRARY_SUFFIX}")
#endif()
#
#set(CMAKE_MODULE_PATH "${COMMON_LOCAL};/usr/local;${CMAKE_MODULE_PATH}")
ExternalProject_Add(project_exiv2
        DEPENDS expat
        DEPENDS gettext
        DEPENDS zlib
        DEPENDS curl
        INSTALL_DIR ${COMMON_LOCAL}
        #URL https://github.com/lrosenth/exiv2/archive/refs/tags/IIIF_V10.zip
        GIT_REPOSITORY https://github.com/Exiv2/exiv2.git
        GIT_TAG main
        SOURCE_DIR ${COMMON_SRCS}/exiv2

        #CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF -DEXIV2_ENABLE_VIDEO=ON -DCMAKE_MODULE_PATH=${COMMON_LOCAL} -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL} -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
        CMAKE_ARGS -DBUILD_SHARED_LIBS:BOOL=${MAKE_SHARED_CSERVE}
            -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL}
            -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
            -DEXIV2_ENABLE_XMP:BOOL=ON
            -DEXIV2_BUILD_SAMPLES:BOOL=OFF
            -DEXIV2_BUILD_EXIV2_COMMAND:BOOL=OFF
            -DEXIV2_BUILD_UNIT_TESTS:BOOL=OFF
            -DEXIV2_BUILD_FUZZ_TESTS:BOOL=OFF

        BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(project_exiv2 install_dir)

if(MAKE_SHARED_SIPI)
    add_library(exiv2 SHARED IMPORTED GLOBAL)
    set_property(TARGET exiv2 PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libexiv2${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(exiv2 STATIC IMPORTED GLOBAL)
    set_property(TARGET exiv2 PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libexiv2${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

add_dependencies(exiv2 project_exiv2)

