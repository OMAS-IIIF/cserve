cmake_minimum_required(VERSION 3.19)
project(libcatch2_builder C CXX)
include(ExternalProject)

#
# get fmt
#
ExternalProject_Add(
        project_catch2
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://github.com/catchorg/Catch2/archive/refs/heads/devel.zip
        SOURCE_DIR ${COMMON_SRCS}/Catch2-devel
        CMAKE_ARGS -DCMAKE_PREFIX_PATH=${COMMON_LOCAL} -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL} -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
)

ExternalProject_Get_Property(project_catch2 install_dir)

if(MAKE_SHARED_SIPI)
    add_library(Catch2 SHARED IMPORTED GLOBAL)
    set_property(TARGET Catch2 PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libCatch2${CMAKE_SHARED_LIBRARY_SUFFIX})
    add_library(Catch2Main SHARED IMPORTED GLOBAL)
    set_property(TARGET Catch2Main PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libCatch2Main${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(Catch2 STATIC IMPORTED GLOBAL)
    set_property(TARGET Catch2 PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libCatch2${CMAKE_STATIC_LIBRARY_SUFFIX})
    add_library(Catch2Main STATIC IMPORTED GLOBAL)
    set_property(TARGET Catch2Main PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libCatch2Main${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

add_dependencies(Catch2 project_catch2)
add_dependencies(Catch2Main project_catch2)
