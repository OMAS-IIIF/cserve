cmake_minimum_required(VERSION 3.19)
message(${CSERVER_EXE})
message(${PROJECT_BINARY_DIR})

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

add_executable (tests_00 test_00.cpp)

include_directories(
        ${COMMON_LOCAL}/include
        /usr/local/include
)
target_link_libraries(tests_00
        cserve
        spdlog
        Catch2Main
        Catch2
        magic
        zlib
        xz
        bzip2
        ssl
        crypto
        Threads::Threads
        ${CMAKE_DL_LIBS})


add_executable (tests_01 test_01.cpp)

include_directories(
        ${COMMON_LOCAL}/include
        /usr/local/include
)

target_link_libraries(tests_01
        cserve
        Catch2Main
        Catch2
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
        Threads::Threads
        ${CMAKE_DL_LIBS})


if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(tests_01 ${COREFOUNDATION_FRAMEWORK} ${SYSTEMCONFIGURATION_FRAMEWORK})
endif()

add_test(NAME tests_00 COMMAND tests_00 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_test(NAME tests_01 COMMAND tests_01 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_test(NAME e2e
        COMMAND pytest -s --cserver=${CSERVER_EXE} --handlerdir=${PROJECT_BINARY_DIR}/handler
        WORKING_DIRECTORY  ${PROJECT_SOURCE_DIR}/tests)


enable_testing()