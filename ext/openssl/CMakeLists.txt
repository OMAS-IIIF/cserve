cmake_minimum_required(VERSION 3.19)
project(project_openssl C CXX)
include(ExternalProject)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    if (${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "x86_64")
        set(CONF_ARCHOPT darwin64-x86_64-cc)
    else()
        set(CONF_ARCHOPT darwin64-arm64-cc)
    endif()
else()
    set(CONF_ARCHOPT "")
endif()
message("CONF_ARCHOPT=${CONF_ARCHOPT}")

ExternalProject_Add(project_openssl
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://www.openssl.org/source/openssl-3.0.0.tar.gz
        SOURCE_DIR ${COMMON_SRCS}/openssl
        CONFIGURE_COMMAND ${COMMON_SRCS}/openssl/Configure ${CONF_ARCHOPT}
        --prefix=${COMMON_LOCAL}
        --libdir=${CONFIGURE_LIBDIR}
        BUILD_COMMAND make
        INSTALL_COMMAND make INSTALL_LIB=${CONFIGURE_LIBDIR} install INSTALL_TOP=${COMMON_LOCAL}
        BUILD_IN_SOURCE 1)

ExternalProject_Get_Property(project_openssl install_dir)

if(MAKE_SHARED_CSERVE)
    #add_library(crypto SHARED IMPORTED GLOBAL)
    #set_property(TARGET crypto PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libcrypto${CMAKE_SHARED_LIBRARY_SUFFIX})
    add_library(openssl SHARED IMPORTED GLOBAL)
    set_property(TARGET openssl PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libsll${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    #add_library(crypto STATIC IMPORTED GLOBAL)
    #set_property(TARGET crypto PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libcrypto${CMAKE_STATIC_LIBRARY_SUFFIX})
    add_library(openssl STATIC IMPORTED GLOBAL)
    set_property(TARGET openssl PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libsll${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

#add_dependencies(crypto project_openssl)
add_dependencies(openssl project_openssl)
