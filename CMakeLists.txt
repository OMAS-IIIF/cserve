cmake_minimum_required(VERSION 3.19)

include(ExternalProject)
include(CheckStructHasMember)
include(CheckIncludeFiles)

project(cserver VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(COMMON_LOCAL ${CMAKE_BINARY_DIR}/local)
set(COMMON_SRCS ${CMAKE_BINARY_DIR}/extsrcs)
set(CONFIGURE_LIBDIR ${COMMON_LOCAL}/lib)
set(CONFIGURE_INCDIR ${COMMON_LOCAL}/include)

# set cmake default install location
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${COMMON_LOCAL}" CACHE PATH "default install path" FORCE)
    message(STATUS CMAKE_INSTALL_DIR: ${CMAKE_INSTALL_PREFIX})
endif()

set(COMMON_SOURCE_FILES_DIR ${PROJECT_SOURCE_DIR}/src)
set(COMMON_INCLUDE_FILES_DIR ${PROJECT_SOURCE_DIR}/include)
#set(DOXYGEN_FILES_DIR ${PROJECT_SOURCE_DIR}/doxygen)
set(EXT_PROJECTS_DIR ${PROJECT_SOURCE_DIR}/ext)
#set(COMMON_PATCHES ${PROJECT_SOURCE_DIR}/patches)
#set(COMMON_VENDOR ${PROJECT_SOURCE_DIR}/vendor)
#set(SIPI_SOURCE_DIR ${PROJECT_SOURCE_DIR})

#
# set path to special find_package() code
#
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_ROOT}/Modules/")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

#
# OpenSSL ??
#
include(FindOpenSSL)
if(OPENSSL_FOUND)
    message(STATUS "Found OpenSSL at ${OPENSSL_ROOT_DIR}")
    add_definitions(${CMAKE_C_FLAGS} " -DSHTTPS_ENABLE_SSL")
    add_definitions(${CMAKE_C_FLAGS} " -I${OPENSSL_INCLUDE_DIR}")
    message(STATUS "OPENSSL_INCLUDE_DIR=${OPENSSL_INCLUDE_DIR}")
    message(STATUS "OPENSSL_VERSION=${OPENSSL_VERSION}")
else()
    message(STATUS "No SSL support found!")
endif()

#
# libmagic
# (used to be build locally from source was moved to prerequisites)
#
find_package(LibMagic REQUIRED)


add_subdirectory(${EXT_PROJECTS_DIR}/lua)
add_subdirectory(${EXT_PROJECTS_DIR}/sqlite3)
add_subdirectory(${EXT_PROJECTS_DIR}/jansson)

add_subdirectory(lib)

link_directories(
        ${CONFIGURE_LIBDIR}
        /usr/local/lib
)

include_directories(
        ${COMMON_LOCAL}/include
        /usr/local/include
)

add_executable(cserver
        ${COMMON_SOURCE_FILES_DIR}/cserver.cpp)

configure_file(${COMMON_SOURCE_FILES_DIR}/CserverVersion.h.in ${COMMON_SOURCE_FILES_DIR}/CserverVersion.h)

target_link_libraries(cserver
        cserve
        magic
        lua
        jansson
        sqlite3
        curl)

if(OPENSSL_FOUND)
    target_link_libraries(cserver ${OPENSSL_LIBRARIES})
endif()


target_include_directories(
        cserver PUBLIC
        "${PROJECT_BINARY_DIR}"
        "${PROJECT_SOURCE_DIR}/lib"
)