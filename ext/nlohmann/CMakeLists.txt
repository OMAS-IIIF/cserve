cmake_minimum_required(VERSION 3.19)
project(nlohmann_builder C CXX)
include(ExternalProject)

ExternalProject_Add(project_nlohmann
        INSTALL_DIR ${COMMON_LOCAL}
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        DOWNLOAD_EXTRACT_TIMESTAMP NOW
        SOURCE_DIR ${COMMON_SRCS}/nlohmann
        CMAKE_ARGS -DJSON_BuildTests:BOOL=OFF
        -DCMAKE_PREFIX_PATH=${COMMON_LOCAL}
        -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL}
        -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
)
ExternalProject_Get_Property(project_nlohmann install_dir)

add_library(nlohmann INTERFACE)

add_dependencies(nlohmann project_nlohmann)
