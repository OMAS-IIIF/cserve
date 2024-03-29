cmake_minimum_required(VERSION 3.14.5)
project(sqlite_builder C CXX)
include(ExternalProject)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.24.0)
    set(timestamp_policy DOWNLOAD_EXTRACT_TIMESTAMP OLD)
else()
    unset(timestamp_policy)
endif()

#https://www.sqlite.org/2024/sqlite-autoconf-3450100.tar.gz
# get sqlite3
#
ExternalProject_Add(project_sqlite3
        INSTALL_DIR ${COMMON_LOCAL}
        URL "https://www.sqlite.org/2024/sqlite-autoconf-3450100.tar.gz"
        ${timestamp_policy}
        SOURCE_DIR ${COMMON_SRCS}/sqlite
        CONFIGURE_COMMAND ${COMMON_SRCS}/sqlite/configure --enable-fts5  --prefix=${COMMON_LOCAL} --enable-shared --enable-static --libdir=${CONFIGURE_LIBDIR}
        BUILD_COMMAND make CFLAGS=-DSQLITE_ENABLE_RTREE=1
        BUILD_IN_SOURCE 1
        )
ExternalProject_Get_Property(project_sqlite3 install_dir)

if(MAKE_SHARED_SIPI)
    add_library(sqlite3 SHARED IMPORTED GLOBAL)
    set_property(TARGET sqlite3 PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libsqlite3${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(CMAKE_CREATE_SHARED "-DBUILD_SHARED_LIBS=ON")
else()
    add_library(sqlite3 STATIC IMPORTED GLOBAL)
    set_property(TARGET sqlite3 PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libsqlite3${CMAKE_STATIC_LIBRARY_SUFFIX})
    set(CMAKE_CREATE_SHARED "-DBUILD_SHARED_LIBS=OFF")
endif()

add_dependencies(sqlite3 project_sqlite3)
