cmake_minimum_required(VERSION 3.19)
project(libbzip2_builder C CXX)
include(ExternalProject)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.24.0)
    set(timestamp_policy DOWNLOAD_EXTRACT_TIMESTAMP OLD)
else()
    unset(timestamp_policy)
endif()

#
# get
#
ExternalProject_Add(project_bzip2
        INSTALL_DIR ${COMMON_LOCAL}
        GIT_REPOSITORY https://sourceware.org/git/bzip2.git
        SOURCE_DIR ${COMMON_SRCS}/bzip2
        CONFIGURE_COMMAND ls
        BUILD_COMMAND make all
        INSTALL_COMMAND make install PREFIX=${COMMON_LOCAL}
        BUILD_IN_SOURCE 1)

ExternalProject_Get_Property(project_bzip2 install_dir)

add_library(bzip2 STATIC IMPORTED GLOBAL)
set_property(TARGET bzip2 PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libbz2${CMAKE_STATIC_LIBRARY_SUFFIX})

add_dependencies(bzip2 project_bzip2)
