# https://storage.googleapis.com/downloads.webmproject.org/releases/webp/libwebp-1.1.0.tar.gz
cmake_minimum_required(VERSION 3.14.5)
project(libwebp_builder C CXX)
include(ExternalProject)

#
# libwebp
#
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.24.0)
    set(timestamp_policy DOWNLOAD_EXTRACT_TIMESTAMP OLD)
else()
    unset(timestamp_policy)
endif()

ExternalProject_Add(project_webp
        DEPENDS xz
        DEPENDS lerc
        DEPENDS sharpyuv
        INSTALL_DIR ${COMMON_LOCAL}
        GIT_REPOSITORY https://github.com/webmproject/libwebp.git
        GIT_TAG main
        SOURCE_DIR ${COMMON_SRCS}/libwebp-latest
        CMAKE_ARGS -DCMAKE_PREFIX_PATH=${COMMON_LOCAL}
        -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL}
        )
ExternalProject_Get_Property(project_webp install_dir)

if(MAKE_SHARED_SIPI)
    add_library(webp SHARED IMPORTED GLOBAL)
    set_property(TARGET webp PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libwebp${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(webp STATIC IMPORTED GLOBAL)
    set_property(TARGET webp PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libwebp${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

if(MAKE_SHARED_SIPI)
    add_library(sharp SHARED IMPORTED GLOBAL)
    set_property(TARGET sharp PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libsharp${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(sharp STATIC IMPORTED GLOBAL)
    set_property(TARGET sharp PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libsharp${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

add_dependencies(webp project_webp)
add_dependencies(sharp project_webp)
