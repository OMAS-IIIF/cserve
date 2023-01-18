cmake_minimum_required(VERSION 3.14.5)
project(lcms2_builder C CXX)
include(ExternalProject)

add_compile_options(-DCMS_NO_REGISTER_KEYWORD=1)
#
# get lcms2
#
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.24.0)
    set(timestamp_policy DOWNLOAD_EXTRACT_TIMESTAMP OLD)
else()
    unset(timestamp_policy)
endif()

ExternalProject_Add(project_lcms2
        DEPENDS turbojpeg
        DEPENDS tiff
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://sourceforge.net/projects/lcms/files/latest/download
        ${timestamp_policy}
        SOURCE_DIR ${COMMON_SRCS}/lcms2-latest
        CONFIGURE_COMMAND   ${COMMON_SRCS}/lcms2-latest/configure
                            --prefix=${COMMON_LOCAL}
                            --with-jpeg=${COMMON_LOCAL}
                            --with-tiff=${COMMON_LOCAL}
                            --libdir=${CONFIGURE_LIBDIR}
        BUILD_COMMAND make
        INSTALL_COMMAND make PREFIX=${COMMON_LOCAL} install
        BUILD_IN_SOURCE 1
        )

ExternalProject_Get_Property(project_lcms2 install_dir)

add_library(lcms2 STATIC IMPORTED GLOBAL)
set_property(TARGET lcms2 PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/liblcms2${CMAKE_STATIC_LIBRARY_SUFFIX})

add_dependencies(lcms2 project_lcms2)
