cmake_minimum_required(VERSION 3.19)

add_executable (tests test_00.cpp)

include_directories(
        ${COMMON_LOCAL}/include
        /usr/local/include
)
target_link_libraries(tests
        cserve
        magic
        lua
        jansson
        sqlite3
        curl
        spdlog
        Catch2 Catch2Main)


if(OPENSSL_FOUND)
    target_link_libraries(tests ${OPENSSL_LIBRARIES})
endif()
add_test (NAME tests COMMAND tests)
enable_testing()