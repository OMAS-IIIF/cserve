//
// Created by Lukas Rosenthaler on 29.06.21.
//
#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch_all.hpp"

#include <iostream>

#include "Error.h"

TEST_CASE("Testing basic things", "[basic]") {
    std::string msg("test message");
    cserve::Error err(__FILE__, __LINE__, msg);
    REQUIRE(err.getMessage() == msg);
}