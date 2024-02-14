cmake_minimum_required(VERSION 3.19)
project(libspdlog_builder C CXX)
include(ExternalProject)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.24.0)
    set(timestamp_policy DOWNLOAD_EXTRACT_TIMESTAMP OLD)
else ()
    unset(timestamp_policy)
endif ()

#
# get spdlog
#
ExternalProject_Add(project_spdlog
        DEPENDS fmt
        INSTALL_DIR ${COMMON_LOCAL}
        #URL https://github.com/gabime/spdlog/archive/refs/heads/v2.x.zip
        URL https://github.com/gabime/spdlog/archive/refs/tags/v1.13.0.tar.gz
        ${timestamp_policy}
        SOURCE_DIR ${COMMON_SRCS}/spdlog-1.13.0.x
        CMAKE_ARGS
        #-DSPDLOG_INSTALL:BOOL=ON
        #-DSPDLOG_SYSTEM_INCLUDES:BOOL=ON
        #-DSPDLOG_FMT_EXTERNAL:BOOL=ON
        -DCMAKE_PREFIX_PATH=${COMMON_LOCAL}
        -DCMAKE_INSTALL_PREFIX=${COMMON_LOCAL}
        -DCMAKE_INSTALL_LIBDIR=${CONFIGURE_LIBDIR}
)

ExternalProject_Get_Property(project_spdlog install_dir)

#ExternalProject_Add_Step(
#        project_spdlog copy_fmt_header
#        COMMAND ${CMAKE_COMMAND} -E copy ${COMMON_SRCS}/spdlog-v2.x/include/spdlog/fmt/fmt.h ${COMMON_LOCAL}/include/fmt/
#        DEPENDEES install # This ensures the copy step is executed after the install step
#)

if(MAKE_SHARED_CSERVE)
    add_library(spdlog SHARED IMPORTED GLOBAL)
    set_property(TARGET spdlog PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libspdlog${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(spdlog STATIC IMPORTED GLOBAL)
    set_property(TARGET spdlog PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libspdlog${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

add_dependencies(spdlog project_spdlog)
