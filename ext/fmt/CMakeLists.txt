cmake_minimum_required(VERSION 3.19)
project(libfmt_builder C CXX)
include(ExternalProject)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
#set(CMAKE_CC_FLAGS "${CMAKE_CC_FLAGS} -fPIC")
#
# get fmt
#
ExternalProject_Add(
        project_fmt
        INSTALL_DIR ${COMMON_LOCAL}
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        SOURCE_DIR ${COMMON_SRCS}/fmt-master
        CMAKE_ARGS -DFMT_DOC=OFF
        -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE
        -DCMAKE_PREFIX_PATH=${COMMON_LOCAL}
        -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL}
        -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
)

ExternalProject_Get_Property(project_fmt install_dir)

if(MAKE_SHARED_CSERVE)
    add_library(fmt SHARED IMPORTED GLOBAL)
    set_property(TARGET fmt PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libfmt${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(fmt STATIC IMPORTED GLOBAL)
    set_property(TARGET fmt PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libfmt${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

add_dependencies(fmt project_fmt)
