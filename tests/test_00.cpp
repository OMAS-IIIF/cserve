//
// Created by Lukas Rosenthaler on 29.06.21.
//
#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch_all.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>

#include <iostream>

#include "Error.h"
#include "SockStream.h"
#include "Hash.h"

TEST_CASE("Testing Error class", "[Error]") {
    std::string msg("test message");
    cserve::Error err(__FILE__, __LINE__, msg); int l = __LINE__;
    REQUIRE(err.getMessage() == msg);
    REQUIRE(err.getLine() == l);
}

TEST_CASE("Testing socket stream", "[SockStream]") {
    int socketfd[2];
    std::string teststr("0123456789|0123456789|0123456789|0123456789|0123456789|0123456789|0123456789|");
    socketpair(AF_UNIX, SOCK_STREAM, 0, socketfd);
    pid_t pid;
    pid = fork();
    if (pid == 0) {
        // child process
        close(socketfd[1]);
        cserve::SockStream sockstreamA(socketfd[0], 40, 40);
        std::ostream os(&sockstreamA);
        os << teststr;
        os.flush();
        close(socketfd[0]);
        exit(0);
    } else {
        // master process
        close(socketfd[0]);
        cserve::SockStream sockstreamB(socketfd[1], 40, 40);
        std::istream is(&sockstreamB);
        std::string result;
        is >> result;
        REQUIRE(result == teststr);
        close(socketfd[1]);
    }
}

TEST_CASE("Testing hasing", "[Hash]") {
    std::string teststr("abcdefghijklmnopqrstuvwxyzöäüABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+!");

    SECTION("md5 testing") {
        cserve::Hash h1(cserve::HashType::md5);
        REQUIRE(h1.add_data(teststr.c_str(), teststr.length()));
        REQUIRE(h1.hash() == "9dbfcb6bcd42c4645179e96a6091944d");

        cserve::Hash h2(cserve::HashType::md5);
        REQUIRE(h2.hash_of_file("./testdata/Kleist.txt"));
        REQUIRE(h2.hash() == "2b14d4fdca178fb87e04e478644e2516");
    }

    SECTION("sha1 testing") {
        cserve::Hash h1(cserve::HashType::sha1);
        REQUIRE(h1.add_data(teststr.c_str(), teststr.length()));
        REQUIRE(h1.hash() == "5f51d77b2bc90b38b65b107afead6ccb7b9ff8d6");

        cserve::Hash h2(cserve::HashType::sha1);
        REQUIRE(h2.hash_of_file("./testdata/Kleist.txt"));
        REQUIRE(h2.hash() == "2b5f315e1995e1b4be9912103b9c62ea85df8e2f");
    }

    SECTION("sha256 testing") {
        cserve::Hash h1(cserve::HashType::sha256);
        REQUIRE(h1.add_data(teststr.c_str(), teststr.length()));
        REQUIRE(h1.hash() == "3b30340cf463750f33b4ecad63b041e0a3388023751fbce7eef46222b16e91cf");

        cserve::Hash h2(cserve::HashType::sha256);
        REQUIRE(h2.hash_of_file("./testdata/Kleist.txt"));
        REQUIRE(h2.hash() == "3753f8a3d26aecfa7768e96d5e8df22822193d90d6f2c39eff5ba4f04c02291d");
    }

    SECTION("sha256 testing") {
        cserve::Hash h1(cserve::HashType::sha384);
        REQUIRE(h1.add_data(teststr.c_str(), teststr.length()));
        REQUIRE(h1.hash() == "88019f3d34a9bdabf4f95412b02dcb6c9136e78976ea939e280530a87c32d3b3de5dd8f15e6357f3cc0ad1f3555c24e8");

        cserve::Hash h2(cserve::HashType::sha384);
        REQUIRE(h2.hash_of_file("./testdata/Kleist.txt"));
        REQUIRE(h2.hash() == "d0ab3edb0c994b85081e674c8c24b5b8c48ec06a2c0605b088d6312d372d6aa1b605cf529c69269737c4e300a0358dc2");
    }

    SECTION("sha256 testing") {
        cserve::Hash h1(cserve::HashType::sha512);
        REQUIRE(h1.add_data(teststr.c_str(), teststr.length()));
        REQUIRE(h1.hash() == "7c579d40720aa01ed5cc1fe8adaa19a05d2e78506c96e2042407d3e41e6d080b522bf874b21bb98ded916e4618491c9326c0b6830abf82049a9490623d3ee97d");

        cserve::Hash h2(cserve::HashType::sha512);
        REQUIRE(h2.hash_of_file("./testdata/Kleist.txt"));
        REQUIRE(h2.hash() == "16c4a4e609bda4d34a0706e89dd61663604e3c2c331fcdf940c5f0e474c7a49e65cc2affe8f52cb60c5af6bb4d91a92aee0e0460df572ea484fbf932632540b6");
    }

}