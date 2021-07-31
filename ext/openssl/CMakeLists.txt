cmake_minimum_required(VERSION 3.14.5)
project(openssl_builder C CXX)
include(ExternalProject)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CONFIGURE_COMMAND Configure darwin64-x86_64-cc)
else()
    set(CONFIGURE_COMMAND "config")
endif()

ExternalProject_Add(project_openssl
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://www.openssl.org/source/openssl-1.1.1k.tar.gz
        SOURCE_DIR ${COMMON_SRCS}/openssl
        CONFIGURE_COMMAND ${COMMON_SRCS}/openssl/${CONFIGURE_COMMAND}
        --prefix=${COMMON_LOCAL}
        --libdir=${CONFIGURE_LIBDIR}
        BUILD_COMMAND make
        INSTALL_COMMAND make INSTALL_LIB=${CONFIGURE_LIBDIR} install INSTALL_TOP=${COMMON_LOCAL}
        BUILD_IN_SOURCE 1
        )

if(MAKE_SHARED_SIPI)
    add_library(openssl SHARED IMPORTED GLOBAL)
    set_property(TARGET openssl PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libsll${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(openssl STATIC IMPORTED GLOBAL)
    set_property(TARGET openssl PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libsll${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

add_dependencies(openssl project_openssl)
target_link_libraries(openssl ${install_dir}/sll.lib)