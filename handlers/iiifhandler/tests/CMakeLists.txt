cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(DARWIN "Darwin")
set(LINUX "Linux")
math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")
if(CMAKE_SYSTEM_NAME STREQUAL DARWIN)
    message(STATUS "Compiling for Darwin...")
    message(STATUS ${CMAKE_SYSTEM_NAME})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    SET(CMAKE_MACOSX_RPATH 1)
    if (${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "x86_64")
        SET(KDU_MAKE "Makefile-MAC-x86-${BITS}-gcc")
        SET(KDU_ARCH "Mac-x86-${BITS}-gcc")
        SET(KDU_EXEC_PLATFORM "Mac-x86-64-gcc")
    else()
        SET(KDU_MAKE "Makefile-MAC-arm-${BITS}-gcc")
        SET(KDU_ARCH "Mac-arm-${BITS}-gcc")
        SET(KDU_EXEC_PLATFORM "Mac-arm-64-gcc")
    endif()
    SET(LUA_ARCH "macosx")
elseif(CMAKE_SYSTEM_NAME STREQUAL LINUX)
    message(STATUS "Compiling for Linux64...")
    message(STATUS ${CMAKE_SYSTEM_NAME})
    execute_process(COMMAND /usr/bin/lsb_release -si COMMAND tr -d '\n' OUTPUT_VARIABLE CMAKE_DISTRO_NAME)
    MESSAGE(STATUS "Distro: " ${CMAKE_DISTRO_NAME})
    if (${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "x86_64")
        SET(KDU_MAKE "Makefile-Linux-x86-${BITS}-gcc")
        SET(KDU_ARCH "Linux-x86-${BITS}-gcc")
        SET(KDU_EXEC_PLATFORM "Linux-x86-64-gcc")
    else()
        SET(KDU_MAKE "Makefile-Linux-arm-${BITS}-gcc")
        SET(KDU_ARCH "Linux-arm-${BITS}-gcc")
        SET(KDU_EXEC_PLATFORM "Linux-arm-64-gcc")

    endif()
    SET(LUA_ARCH "linux")
endif()

add_executable (iiif_tests_01 test_01.cpp
        ../IIIFError.cpp ../IIIFError.h
        ../iiifparser/IIIFRegion.cpp ../iiifparser/IIIFRegion.h
        ../iiifparser/IIIFSize.cpp ../iiifparser/IIIFSize.h)

include_directories(
        ${CMAKE_SOURCE_DIR}/lib
        ${COMMON_LOCAL}/include
        /usr/local/include
)

target_link_libraries(iiif_tests_01 PRIVATE
        cserve
        Catch2Main
        Catch2
        fmt
        magic
        lua
        sqlite3
        jwtcpp
        spdlog
        curl
        ssl
        crypto
        zlib
        xz
        bzip2
        exiv2
        tiff
        turbojpeg
        png
        kdu
        kdu_aux
        expat
        lcms2
        iconv
        gettext_intl
        zlib
        zstd
        Threads::Threads
        ${CMAKE_DL_LIBS})

add_test(NAME iiif_tests_01 COMMAND iiif_tests_01 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

enable_testing()

