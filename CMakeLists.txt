cmake_minimum_required(VERSION 3.19)

include(ExternalProject)
include(CheckStructHasMember)
include(CheckIncludeFiles)
include(FindPackageHandleStandardArgs)

project(cserver VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

option(MAKE_SHARED_CSERVE "Create cserver using all shared libraries" OFF)
option(IIIF_HANDLER "Add IIIF Handler" ON)

set(COMMON_LOCAL ${CMAKE_BINARY_DIR}/local)
set(COMMON_SRCS ${CMAKE_BINARY_DIR}/extsrcs)
set(CONFIGURE_LIBDIR ${COMMON_LOCAL}/lib)
set(CONFIGURE_INCDIR ${COMMON_LOCAL}/include)
set(COMMON_VENDOR ${PROJECT_SOURCE_DIR}/vendor)

set(DARWIN "Darwin")
set(LINUX "Linux")
math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")
if(CMAKE_SYSTEM_NAME STREQUAL DARWIN)
    message(STATUS "Compiling for Darwin...")
    message(STATUS ${CMAKE_SYSTEM_NAME})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    SET(CMAKE_MACOSX_RPATH 1)
    if (${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "x86_64")
        SET(KDU_MAKE "Makefile-Mac-x86-${BITS}-gcc")
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
message(STATUS "Compiling for ${KDU_ARCH}")

set(CSERVER_EXE ${CMAKE_CURRENT_BINARY_DIR}/cserver)


set(COMMON_SOURCE_FILES_DIR ${PROJECT_SOURCE_DIR}/src)
set(COMMON_LIBRARY_FILES_DIR ${PROJECT_SOURCE_DIR}/lib)
set(COMMON_INCLUDE_FILES_DIR ${PROJECT_SOURCE_DIR}/include)

#set(DOXYGEN_FILES_DIR ${PROJECT_SOURCE_DIR}/doxygen)
set(EXT_PROJECTS_DIR ${PROJECT_SOURCE_DIR}/ext)

#
# set path to special find_package() code
#
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_ROOT}/Modules/")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")


add_definitions(${CMAKE_CXX_FLAGS} " -std=c++17")
#
# this is needed for curl...
#
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_library(COREFOUNDATION_FRAMEWORK "CoreFoundation")
    if(NOT COREFOUNDATION_FRAMEWORK)
        message(FATAL_ERROR "CoreFoundation framework not found")
    endif()

    find_library(SYSTEMCONFIGURATION_FRAMEWORK "SystemConfiguration")
    if(NOT SYSTEMCONFIGURATION_FRAMEWORK)
        message(FATAL_ERROR "SystemConfiguration framework not found")
    endif()

    list(APPEND CURL_LIBS "-framework CoreFoundation" "-framework SystemConfiguration")

    if(CMAKE_USE_SECTRANSP)
        find_library(SECURITY_FRAMEWORK "Security")
        if(NOT SECURITY_FRAMEWORK)
            message(FATAL_ERROR "Security framework not found")
        endif()

        set(SSL_ENABLED ON)
        set(USE_SECTRANSP ON)
        list(APPEND CURL_LIBS "-framework Security")
    endif()
endif()




set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if(CMAKE_SYSTEM_NAME STREQUAL DARWIN)
    find_package(Iconv REQUIRED)
endif()

#message(STATUS "--->iconv libraries: ${ICONV_LIBRARIES}")

add_subdirectory(${EXT_PROJECTS_DIR}/fmt)
add_subdirectory(${EXT_PROJECTS_DIR}/spdlog)
add_subdirectory(${EXT_PROJECTS_DIR}/openssl)
add_subdirectory(${EXT_PROJECTS_DIR}/curl)
add_subdirectory(${EXT_PROJECTS_DIR}/lua)
add_subdirectory(${EXT_PROJECTS_DIR}/sqlite3)
add_subdirectory(${EXT_PROJECTS_DIR}/nlohmann)
add_subdirectory(${EXT_PROJECTS_DIR}/jwt-cpp)
add_subdirectory(${EXT_PROJECTS_DIR}/zlib)
add_subdirectory(${EXT_PROJECTS_DIR}/xz)
add_subdirectory(${EXT_PROJECTS_DIR}/bzip2)
add_subdirectory(${EXT_PROJECTS_DIR}/magic)
add_subdirectory(${EXT_PROJECTS_DIR}/catch2)
add_subdirectory(lib)
add_subdirectory(handlers/testhandler)
add_subdirectory(handlers/pinghandler)
add_subdirectory(handlers/scripthandler)
add_subdirectory(handlers/filehandler)
add_subdirectory(handlers/iiifhandler)


link_directories(
        ${CONFIGURE_LIBDIR}
        /usr/local/lib
)

include_directories(
        ${COMMON_LOCAL}/include
        /usr/local/include
)

add_executable(cserver
        ${COMMON_SOURCE_FILES_DIR}/cserver.cpp)

configure_file(${COMMON_LIBRARY_FILES_DIR}/CserveVersion.h.in ${COMMON_LIBRARY_FILES_DIR}/CserveVersion.h)

target_link_libraries(cserver
        cserve
        testhandler
        pinghandler
        filehandler
        scripthandler
        iiifhandler
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
        tiff
        turbojpeg
        png
        webp
        lerc
        jbigkit
        exiv2
        kdu_aux
        kdu
        expat
        lcms2
        xz
        zlib
        zstd
        deflate
        sharpyuv
        cserve
        Threads::Threads
        ${CMAKE_DL_LIBS})

if(CMAKE_SYSTEM_NAME STREQUAL DARWIN)
    target_link_libraries(cserver
            ${COREFOUNDATION_FRAMEWORK}
            ${SYSTEMCONFIGURATION_FRAMEWORK}
            iconv)
endif()


target_include_directories(cserver PUBLIC
        ${PROJECT_BINARY_DIR}
        ${PROJECT_SOURCE_DIR}/lib
)

add_dependencies(cserver
        testhandler
        pinghandler
        scripthandler
        filehandler)

if(${IIIF_HANDLER})
    add_dependencies(cserver
            iiifhandler)
endif()

add_custom_command(TARGET cserver POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:cserver>
        ${COMMON_LOCAL}/bin/cserver)

install(TARGETS cserver cserve RUNTIME ARCHIVE)
install(DIRECTORY ${COMMON_LOCAL}/lib/ TYPE LIB REGEX "/(cmake|lua|pkkconfig)/" EXCLUDE)
install(DIRECTORY ${COMMON_LOCAL}/include/ TYPE INCLUDE)

include(CTest)

add_subdirectory(tests)

enable_testing()
