cmake_minimum_required(VERSION 3.19)
project(nlohmann_builder C CXX)
include(ExternalProject)

ExternalProject_Add(project_nlohmann
        INSTALL_DIR ${COMMON_LOCAL}
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        SOURCE_DIR ${COMMON_SRCS}/nlohmann
        CMAKE_ARGS -DJSON_BuildTests:BOOL=OFF
        -DCMAKE_PREFIX_PATH=${COMMON_LOCAL}
        -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL}
        -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
)
ExternalProject_Get_Property(project_nlohmann install_dir)

if(MAKE_SHARED_CSERVE)
    add_library(nlohmann SHARED IMPORTED GLOBAL)
    set_property(TARGET nlohmann PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libnlohmann${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(nlohmann STATIC IMPORTED GLOBAL)
    set_property(TARGET nlohmann PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libnlohmann${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

add_dependencies(nlohmann project_nlohmann)
