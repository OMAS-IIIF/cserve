cmake_minimum_required(VERSION 3.14.5)
project(liblerc_builder C CXX)
include(ExternalProject)

#
# libwebp
#
ExternalProject_Add(project_lerc
        DEPENDS xz
        INSTALL_DIR ${COMMON_LOCAL}
        GIT_REPOSITORY https://github.com/Esri/lerc.git
        SOURCE_DIR ${COMMON_SRCS}/lerc-latest
        CMAKE_ARGS -DCMAKE_PREFIX_PATH=${COMMON_LOCAL}
            -DBUILD_SHARED_LIBS:BOOL=${MAKE_SHARED_CSERVE}
            -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL}
        )
ExternalProject_Get_Property(project_lerc install_dir)

if(MAKE_SHARED_CSERVE)
    add_library(lerc SHARED IMPORTED GLOBAL)
    set_property(TARGET lerc PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libLerc${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(lerc STATIC IMPORTED GLOBAL)
    set_property(TARGET lerc PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libLerc${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

add_dependencies(lerc project_lerc)
