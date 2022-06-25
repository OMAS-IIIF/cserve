//
// Created by Lukas Rosenthaler on 25.06.22.
//
#include "catch2/catch_all.hpp"

#include "CLI11.hpp"
#include "LuaServer.h"
#include "Global.h"

#include "ConfValue.h"


TEST_CASE("Testing ConfValue class", "[ConfValue]") {
    SECTION("Integer testing") {
        std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>();
        std::string description = "This is a description";
        std::string envname = "ITEST";
        int idefval = 4711;
        auto ival = cserve::ConfValue("itest", idefval, description, envname, app);
        REQUIRE(ival.get_int().value() == idefval);
        REQUIRE(ival.get_description() == description);
        REQUIRE(ival.get_envname() == envname);
        int argc = 2;
        char const *argv[] = {"--itest", "42"};
        app->parse(argc, argv);
        REQUIRE(ival.get_int().value() == 42);
        REQUIRE_THROWS_AS(ival.get_float().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(ival.get_string().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(ival.get_datasize().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(ival.get_loglevel().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(ival.get_loglevel_as_string().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(ival.get_luaroutes().value(), std::bad_optional_access);
    }

    SECTION("Float testing") {
        std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>();
        std::string description = "This is a description";
        std::string envname = "FTEST";
        float fdefval = 3.1415;
        auto fval = cserve::ConfValue("ftest", fdefval, description, envname, app);
        REQUIRE(fval.get_float().value() == fdefval);
        REQUIRE(fval.get_description() == description);
        REQUIRE(fval.get_envname() == envname);
        int argc = 2;
        char const *argv[] = {"--ftest", "2.71"};
        app->parse(argc, argv);
        REQUIRE(fval.get_float().value() == 2.71f);
        REQUIRE_THROWS_AS(fval.get_int().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(fval.get_string().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(fval.get_datasize().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(fval.get_loglevel().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(fval.get_loglevel_as_string().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(fval.get_luaroutes().value(), std::bad_optional_access);
    }

    SECTION("char* testing") {
        std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>();
        std::string description = "This is a description";
        std::string envname = "STEST";
        char const *sdefval = "Teststring";
        auto sval = cserve::ConfValue("stest", sdefval, description, envname, app);
        REQUIRE(sval.get_string().value() == std::string(sdefval));
        REQUIRE(sval.get_description() == description);
        REQUIRE(sval.get_envname() == envname);
        int argc = 2;
        char const *argv[] = {"--ftest", "Another string"};
        app->parse(argc, argv);
        REQUIRE(sval.get_string().value() == std::string("Another string"));
        REQUIRE_THROWS_AS(sval.get_int().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(sval.get_float().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(sval.get_datasize().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(sval.get_loglevel().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(sval.get_loglevel_as_string().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(sval.get_luaroutes().value(), std::bad_optional_access);
    }

    SECTION("String testing") {
        std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>();
        std::string description = "This is a description";
        std::string envname = "STEST";
        std::string sdefval = "Teststring";
        auto sval = cserve::ConfValue("stest", sdefval, description, envname, app);
        REQUIRE(sval.get_string().value() == std::string(sdefval));
        REQUIRE(sval.get_description() == description);
        REQUIRE(sval.get_envname() == envname);
        int argc = 2;
        char const *argv[] = {"--ftest", "Another string"};
        app->parse(argc, argv);
        REQUIRE(sval.get_string().value() == std::string("Another string"));
        REQUIRE_THROWS_AS(sval.get_int().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(sval.get_float().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(sval.get_datasize().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(sval.get_loglevel().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(sval.get_loglevel_as_string().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(sval.get_luaroutes().value(), std::bad_optional_access);
    }

    SECTION("DataSize testing") {
        std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>();
        std::string description = "This is a description";
        std::string envname = "DSTEST";
        cserve::DataSize dsdefval("10MB");
        auto sval = cserve::ConfValue("dstest", dsdefval, description, envname, app);
        REQUIRE(sval.get_datasize().value().as_size_t() == dsdefval.as_size_t());
        REQUIRE(sval.get_description() == description);
        REQUIRE(sval.get_envname() == envname);
        int argc = 2;
        char const *argv[] = {"--dstest", "1TB"};
        app->parse(argc, argv);
        REQUIRE(sval.get_datasize().value().as_size_t() == cserve::DataSize("1TB").as_size_t());
        REQUIRE_THROWS_AS(sval.get_int().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(sval.get_float().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(sval.get_string().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(sval.get_loglevel().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(sval.get_loglevel_as_string().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(sval.get_luaroutes().value(), std::bad_optional_access);
    }

}