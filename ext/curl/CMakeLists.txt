cmake_minimum_required(VERSION 3.14.5)
project(curl_builder C CXX)
include(ExternalProject)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
endif()

include(FindOpenSSL REQUIRED)
#
# get libcurl
#
ExternalProject_Add(project_curl
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://curl.haxx.se/download/curl-7.78.0.tar.gz
        SOURCE_DIR ${COMMON_SRCS}/curl-7.78.0
        CMAKE_ARGS  -DCMAKE_PREFIX_PATH=${COMMON_LOCAL}
                    -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL}
                    -DBUILD_SHARED_LIBS:BOOL=OFF
                    -DHTTP_ONLY:BOOL=ON
        )
ExternalProject_Get_Property(project_curl install_dir)
if(MAKE_SHARED_SIPI)
    add_library(curl SHARED IMPORTED GLOBAL)
    set_property(TARGET curl PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libcurl${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(CMAKE_CREATE_SHARED "-DBUILD_SHARED_LIBS=ON")
else()
    add_library(curl STATIC IMPORTED GLOBAL)
    set_property(TARGET curl PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libcurl${CMAKE_STATIC_LIBRARY_SUFFIX})
    set(CMAKE_CREATE_SHARED "-DBUILD_SHARED_LIBS=OFF")
endif()
add_dependencies(curl project_curl)
