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
    #SET(OPENSSL_ROOT_DIR "/usr/local/opt/openssl" CACHE FILEPATH "openssl")
    #SET(OPENSSL_USE_STATIC_LIBS ON)
elseif(CMAKE_SYSTEM_NAME STREQUAL LINUX)
    message(STATUS "Compiling for Linux64...")
    message(STATUS ${CMAKE_SYSTEM_NAME})
    execute_process(COMMAND /usr/bin/lsb_release -si COMMAND tr -d '\n' OUTPUT_VARIABLE CMAKE_DISTRO_NAME)
    MESSAGE(STATUS "Distro: " ${CMAKE_DISTRO_NAME})
endif()

#
# set path to special find_package() code
#
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_ROOT}/Modules/")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

#if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
#    set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
#endif()

#include(FindOpenSSL REQUIRED)
#add_definitions(${CMAKE_CXX_FLAGS} " -I${OPENSSL_INCLUDE_DIR}")

#
# libmagic
# (used to be build locally from source was moved to prerequisites)
#
#find_package(LibMagic REQUIRED)

include_directories(
        ${COMMON_LOCAL}/include
        /usr/local/include
        #${LIBMAGIC_INCLUDES}
)

configure_file(CserveVersion.h.in CserveVersion.h)

add_library(cserve
        CLI11.hpp
        Global.h
        makeunique.h
        Error.cpp Error.h
        SockStream.cpp SockStream.h
        Connection.cpp Connection.h
        ChunkReader.cpp ChunkReader.h
        Hash.cpp Hash.h
        Parsing.cpp Parsing.h
        NlohmannTraits.h
        LuaServer.cpp LuaServer.h
        LuaSqlite.cpp LuaSqlite.h
        SocketControl.cpp SocketControl.h
        ThreadControl.cpp ThreadControl.h
        RequestHandlerData.h
        Cserve.cpp Cserve.h
        RequestHandler.h
        DefaultHandler.cpp DefaultHandler.h
        FileHandler.cpp FileHandler.h
        CserveVersion.h
        RequestHandlerLoader.cpp RequestHandlerLoader.h
        ConfValue.cpp ConfValue.h
        CserverConf.cpp CserverConf.h)

add_dependencies(cserve
        spdlog
        lua
        sqlite3
        jwtcpp
        magic
        nlohmann
        curl)

target_include_directories(cserve
        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
        )

install(TARGETS cserve
        DESTINATION lib/cserve)
install(FILES Global.h Error.h Cserve.h Connection.h CserveVersion.h
        DESTINATION include/cserve)