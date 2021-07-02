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