cmake_minimum_required(VERSION 3.14.5)
project(iconv_builder C CXX)
include(ExternalProject)

#
# get iconf
#
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.24.0)
    set(timestamp_policy DOWNLOAD_EXTRACT_TIMESTAMP OLD)
else()
    unset(timestamp_policy)
endif()

ExternalProject_Add(
        project_iconv
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.17.tar.gz
        ${timestamp_policy}
        SOURCE_DIR ${COMMON_SRCS}/iconv-1.17
        CONFIGURE_COMMAND   ${COMMON_SRCS}/iconv-1.17/configure
                            --prefix=${COMMON_LOCAL}
                            --enable-static
                            --libdir=${CONFIGURE_LIBDIR}
                            --disable-java
        BUILD_COMMAND make
        INSTALL_COMMAND make PREFIX=${COMMON_LOCAL} install
        # INSTALL_COMMAND ${CMAKE_COMMAND} -E copy ${COMMON_SRCS}/gettext--0.20.1/gettext-runtime/intl/.libs/libgnuintl${CMAKE_STATIC_LIBRARY_SUFFIX} ${CONFIGURE_LIBDIR}
        BUILD_IN_SOURCE 1
)

ExternalProject_Get_Property(project_iconv install_dir)

add_library(iconv STATIC IMPORTED GLOBAL)
set_property(TARGET iconv PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libiconv${CMAKE_STATIC_LIBRARY_SUFFIX})

add_dependencies(iconv project_iconv)
