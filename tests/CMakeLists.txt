cmake_minimum_required(VERSION 3.19)
message(${CSERVER_EXE})

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
        ${LIBMAGIC_LIBRARIES})


if(OPENSSL_FOUND)
    target_link_libraries(tests_00 ${OPENSSL_LIBRARIES})
endif()

add_test(NAME tests_00 COMMAND tests_00 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME e2e COMMAND pytest -s --cserver=${CSERVER_EXE} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})


enable_testing()