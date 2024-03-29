cmake_minimum_required(VERSION 3.14.5)
project(jbigkit_builder C CXX)
include(ExternalProject)

#
# get JBIG
#
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.24.0)
    set(timestamp_policy DOWNLOAD_EXTRACT_TIMESTAMP OLD)
else()
    unset(timestamp_policy)
endif()

ExternalProject_Add(
        project_jbigkit
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://www.cl.cam.ac.uk/~mgk25/jbigkit/download/jbigkit-2.1.tar.gz
        ${timestamp_policy}
        SOURCE_DIR ${COMMON_SRCS}//jbigkit-2.1
        CONFIGURE_COMMAND ""
        BUILD_COMMAND make
        INSTALL_COMMAND
            ${CMAKE_COMMAND} -E copy ${COMMON_SRCS}/jbigkit-2.1/libjbig/libjbig${CMAKE_STATIC_LIBRARY_SUFFIX} ${CONFIGURE_LIBDIR} &&
            ${CMAKE_COMMAND} -E copy ${COMMON_SRCS}/jbigkit-2.1/libjbig/jbig.h ${CONFIGURE_INCDIR}/jbig.h &&
            ${CMAKE_COMMAND} -E copy ${COMMON_SRCS}/jbigkit-2.1/libjbig/jbig85.h ${CONFIGURE_INCDIR}/jbig85.h &&
            ${CMAKE_COMMAND} -E copy ${COMMON_SRCS}/jbigkit-2.1/libjbig/jbig_ar.h ${CONFIGURE_INCDIR}/jbig_ar.h
        BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(project_jbigkit install_dir)

add_library(jbigkit STATIC IMPORTED GLOBAL)
set_property(TARGET jbigkit PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libjbig${CMAKE_STATIC_LIBRARY_SUFFIX})

add_dependencies(jbigkit project_jbigkit)
