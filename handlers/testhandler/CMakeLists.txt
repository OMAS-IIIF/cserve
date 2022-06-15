cmake_minimum_required(VERSION 3.19)

project(cserve VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(DARWIN "Darwin")
set(LINUX "Linux")
if(CMAKE_SYSTEM_NAME STREQUAL DARWIN)
    message(STATUS "Compiling for Darwin...")
    message(STATUS ${CMAKE_SYSTEM_NAME})
    SET(CMAKE_MACOSX_RPATH 1)
    SET(LUA_ARCH "macosx")
    # ToDo: shouldn't this be also set on the mac through FindOpenSSL?
elseif(CMAKE_SYSTEM_NAME STREQUAL LINUX)
    message(STATUS "Compiling for Linux64...")
    message(STATUS ${CMAKE_SYSTEM_NAME})
    execute_process(COMMAND /usr/bin/lsb_release -si COMMAND tr -d '\n' OUTPUT_VARIABLE CMAKE_DISTRO_NAME)
    MESSAGE(STATUS "Distro: " ${CMAKE_DISTRO_NAME})
endif()

include_directories(
        ${COMMON_LOCAL}/include
        /usr/local/include
)


add_library(testhandler MODULE
        TestHandler.cpp TestHandler.h )

target_link_libraries(testhandler PRIVATE cserve)



