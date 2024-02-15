cmake_minimum_required(VERSION 3.14.5)
project(kakadu_builder C CXX)
include(ExternalProject)

message(STATUS ${CONFIGURE_LIBDIR})
#
# compiling kakadu (without JAVA interface)
#

message(STATUS "VENDOR=${COMMON_VENDOR}")
message(STATUS "KDU_MAKE=${KDU_MAKE}")
ExternalProject_Add(
        project_kakadu
        INSTALL_DIR ${COMMON_LOCAL}
        DOWNLOAD_COMMAND unzip -o -d ${COMMON_SRCS} ${COMMON_VENDOR}/v8_4_1-01382N.zip
        CONFIGURE_COMMAND ""
        BUILD_COMMAND make --directory=${COMMON_SRCS}/v8_4_1-01382N/make/ -f ${KDU_MAKE} EXEC_PLATFORM=${KDU_EXEC_PLATFORM} all_but_jni
        INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory ${COMMON_SRCS}/v8_4_1-01382N/lib/${KDU_ARCH} ${CONFIGURE_LIBDIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${COMMON_SRCS}/v8_4_1-01382N/bin/${KDU_ARCH} ${COMMON_LOCAL}/bin
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${COMMON_SRCS}/v8_4_1-01382N/managed/all_includes ${COMMON_LOCAL}/include
)

ExternalProject_Get_Property(project_kakadu install_dir)

if(MAKE_SHARED_CSERVE)
    add_library(kdu SHARED IMPORTED GLOBAL)
    set_property(TARGET kdu PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libkdu_v8AR.so)
else()
    add_library(kdu STATIC IMPORTED GLOBAL)
    set_property(TARGET kdu PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libkdu${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

if(MAKE_SHARED_CSERVE)
    add_library(kdu_aux SHARED IMPORTED GLOBAL)
    set_property(TARGET kdu_aux PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libkdu_a8AR.so)
else()
    add_library(kdu_aux STATIC IMPORTED GLOBAL)
    set_property(TARGET kdu_aux PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/libkdu_aux${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

add_dependencies(kdu project_kakadu)
add_dependencies(kdu_aux project_kakadu)
