cmake_minimum_required(VERSION 3.19)


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

add_library(testhandler STATIC
        TestHandler.cpp TestHandler.h )
add_dependencies(testhandler
        cserve
)
#set_target_properties(testhandler PROPERTIES PREFIX "")

#target_link_libraries(testhandler PRIVATE
#        cserve
#        magic
#        lua
#        sqlite3
#        jwtcpp
#        spdlog
#        curl
#        ssl
#        crypto
#        zlib
#        xz
#        bzip2
#        Threads::Threads
#        ${CMAKE_DL_LIBS})

#if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
#    target_link_libraries(testhandler PRIVATE ${COREFOUNDATION_FRAMEWORK} ${SYSTEMCONFIGURATION_FRAMEWORK})
#endif()


#add_custom_command(TARGET testhandler POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy
#        $<TARGET_FILE:testhandler>
#        ${PROJECT_BINARY_DIR}/handler/$<TARGET_FILE_NAME:testhandler>)

#add_custom_command(TARGET testhandler POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy
#        $<TARGET_FILE:testhandler>
#        ${CMAKE_SOURCE_DIR}/serverroot/handler/$<TARGET_FILE_NAME:testhandler>)


#install(TARGETS testhandler DESTINATION ${COMMON_LOCAL}/share/cserver/)