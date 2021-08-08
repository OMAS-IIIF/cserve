cmake_minimum_required(VERSION 3.19)
project(jwtcpp_builder C CXX)
include(ExternalProject)

#
# Setting up openssl - things...
#
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
    set(OS_X_SPECIAL_FLAG -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl)
else()
    set(OS_X_SPECIAL_FLAG "")
endif()
include(FindOpenSSL REQUIRED)

ExternalProject_Add(project_jwtcpp
        INSTALL_DIR ${COMMON_LOCAL}
        GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
        SOURCE_DIR ${COMMON_SRCS}/jwt-cpp
        CMAKE_ARGS -DJWT_BUILD_EXAMPLES:BOOL=OFF
        -DCMAKE_PREFIX_PATH=${COMMON_LOCAL}
        -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL}
        -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
        ${OS_X_SPECIAL_FLAG})
ExternalProject_Get_Property(project_jwtcpp install_dir)

add_library(jwtcpp INTERFACE)

add_dependencies(jwtcpp project_jwtcpp)
