//
// Created by Lukas Rosenthaler on 25.06.22.
//
#include "catch2/catch_all.hpp"

#include "CLI11.hpp"
#include "LuaServer.h"
#include "Global.h"

#include "ConfValue.h"

TEST_CASE("DataSize class", "DataSize") {
    cserve::DataSize ds1(100);
    REQUIRE(ds1.as_size_t() == 100);
    cserve::DataSize ds2("100");
    REQUIRE(ds2.as_size_t() == 100);
    cserve::DataSize ds3("100KB");
    REQUIRE(ds3.as_size_t() == 100*1024);
    cserve::DataSize ds4("100MB");
    REQUIRE(ds4.as_size_t() == 100*1024*1024);
    cserve::DataSize ds5("100GB");
    REQUIRE(ds5.as_size_t() == 100ll*1024ll*1024ll*1024ll);
    cserve::DataSize ds6("100TB");
    REQUIRE(ds6.as_size_t() == 100ll*1024ll*1024ll*1024ll*1024ll);
    cserve::DataSize ds7("100GAGA");
    REQUIRE(ds7.as_size_t() == 100ll);
}

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

    SECTION("loglevel testing") {
        std::string description = "This is a description";
        std::string envname = "LLTEST";
        std::vector<spdlog::level::level_enum> all {
            spdlog::level::level_enum::trace,
            spdlog::level::level_enum::debug,
            spdlog::level::level_enum::info,
            spdlog::level::level_enum::warn,
            spdlog::level::level_enum::err,
            spdlog::level::level_enum::critical,
            spdlog::level::level_enum::off
        };
        std::vector<std::string> allstr {
            "TRACE", "DEBUG", "INFO", "WARN", "ERR", "CRITICAL", "OFF"
        };
        int i = 0;
        for (auto defval: all) {
            std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>();
            auto sval = cserve::ConfValue("test", defval, description, envname, app);
            REQUIRE(sval.get_loglevel().value() == defval);
            REQUIRE(sval.get_loglevel_as_string().value() == allstr[i]);
            REQUIRE(sval.get_description() == description);
            REQUIRE(sval.get_envname() == envname);
            int argc = 2;
            int ii = i >= (all.size() - 1) ? 0 : i + 1;
            char const *argv[] = {"--loglevel", allstr[ii].c_str()};
            app->parse(argc, argv);
            REQUIRE(sval.get_loglevel().value() == all[ii]);
            REQUIRE(sval.get_loglevel_as_string().value() == allstr[ii]);
            REQUIRE_THROWS_AS(sval.get_int().value(), std::bad_optional_access);
            REQUIRE_THROWS_AS(sval.get_float().value(), std::bad_optional_access);
            REQUIRE_THROWS_AS(sval.get_string().value(), std::bad_optional_access);
            REQUIRE_THROWS_AS(sval.get_datasize().value(), std::bad_optional_access);
            REQUIRE_THROWS_AS(sval.get_luaroutes().value(), std::bad_optional_access);
            ++i;
        }
    }

    SECTION("LuaRoutes testing") {
        std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>();
        std::string description = "This is a description";
        std::string envname = "LRTEST";
        std::vector<cserve::LuaRoute> lrdefval {
                cserve::LuaRoute("GET:/gaga:gaga.lua"),
                cserve::LuaRoute("PUT:/gugus:gugus.lua"),
        };
        auto sval = cserve::ConfValue("lrtest", lrdefval, description, envname, app);
        REQUIRE(sval.get_luaroutes().value() == lrdefval);
        REQUIRE(sval.get_description() == description);
        REQUIRE(sval.get_envname() == envname);
        int argc = 3;
        char const *argv[] = {"--lrtest", "GET:/hello:hello.lua", "PUT:/byebye:byebye.lua"};
        app->parse(argc, argv);
        auto result = sval.get_luaroutes().value();
        REQUIRE(result.size() == 2);
        REQUIRE(result[0] == cserve::LuaRoute("GET:/hello:hello.lua"));
        REQUIRE(result[1] == cserve::LuaRoute("PUT:/byebye:byebye.lua"));
        REQUIRE_THROWS_AS(sval.get_int().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(sval.get_float().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(sval.get_string().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(sval.get_loglevel().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(sval.get_loglevel_as_string().value(), std::bad_optional_access);
        REQUIRE_THROWS_AS(sval.get_datasize().value(), std::bad_optional_access);
    }

    SECTION("Environment testing for int") {
        std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>();
        std::string description = "This is a description";
        std::string envname = "ITEST";
        int idefval = 4711;
        auto ival = cserve::ConfValue("itest", idefval, description, envname, app);
        putenv((char *) "ITEST=1234");
        int argc = 1;
        char const *argv[] = {"program"};
        app->parse(argc, argv);
        REQUIRE(ival.get_int().value() == 1234);
    }

    SECTION("Environment testing for float") {
        std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>();
        std::string description = "This is a description";
        std::string envname = "FTEST";
        float fdefval = 3.1415f;
        auto ival = cserve::ConfValue("ftest", fdefval, description, envname, app);
        putenv((char *) "FTEST=2.67");
        int argc = 1;
        char const *argv[] = {"program"};
        app->parse(argc, argv);
        REQUIRE(ival.get_float().value() == 2.67f);
    }

    SECTION("Environment testing for string") {
        std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>();
        std::string description = "This is a description";
        std::string envname = "STEST";
        std::string sdefval = "soso lala";
        auto ival = cserve::ConfValue("ftest", sdefval, description, envname, app);
        putenv((char *) "STEST=was soll denn das?");
        int argc = 1;
        char const *argv[] = {"program"};
        app->parse(argc, argv);
        REQUIRE(ival.get_string().value() == std::string("was soll denn das?"));
    }

    SECTION("Environment testing for DataSize") {
        std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>();
        std::string description = "This is a description";
        std::string envname = "DSTEST";
        cserve::DataSize dsdefval("1MB");
        auto dsval = cserve::ConfValue("dstest", dsdefval, description, envname, app);
        putenv((char *) "DSTEST=2GB");
        int argc = 1;
        char const *argv[] = {"program"};
        app->parse(argc, argv);
        REQUIRE(dsval.get_datasize().value().as_string() == std::string("2GB"));
    }

    SECTION("Environment testing for loglevel") {
        std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>();
        std::string description = "This is a description";
        std::string envname = "LLTEST";
        spdlog::level::level_enum lldefval = spdlog::level::level_enum::info;
        auto dsval = cserve::ConfValue("dstest", lldefval, description, envname, app);
        putenv((char *) "LLTEST=ERR");
        int argc = 1;
        char const *argv[] = {"program"};
        app->parse(argc, argv);
        REQUIRE(dsval.get_loglevel().value() == spdlog::level::level_enum::err);
    }

    SECTION("Environment testing for LuaRoutes") {
        std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>();
        std::string description = "This is a description";
        std::string envname = "LRTEST";
        std::vector<cserve::LuaRoute> lrdefval {
                cserve::LuaRoute("GET:/gaga:gaga.lua"),
                cserve::LuaRoute("PUT:/gugus:gugus.lua"),
        };
        auto lrval = cserve::ConfValue("lrtest", lrdefval, description, envname, app);
        putenv((char *) "LRTEST=POST:/hoppla:hoppla.lua;DELETE:/delme/delme.lua");
        int argc = 1;
        char const *argv[] = {"program"};
        app->parse(argc, argv);
        auto result = lrval.get_luaroutes().value();
        REQUIRE(result.size() == 2);
        REQUIRE(result[0] == cserve::LuaRoute("POST:/hoppla:hoppla.lua"));
        REQUIRE(result[1] == cserve::LuaRoute("DELETE:/delme/delme.lua"));
    }

}