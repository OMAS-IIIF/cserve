//
// Created by Lukas Rosenthaler on 25.06.22.
//
#include "catch2/catch_all.hpp"

#include "CLI11.hpp"
#include "LuaServer.h"
#include "Global.h"
#include "Connection.h"

#include "ConfValue.h"
#include "CserverConf.h"

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
        auto ival = cserve::ConfValue("cserve", "itest", idefval, description, envname, app);
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
        auto fval = cserve::ConfValue("cserve", "ftest", fdefval, description, envname, app);
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
        auto sval = cserve::ConfValue("cserve", "stest", sdefval, description, envname, app);
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
        auto sval = cserve::ConfValue("cserve", "stest", sdefval, description, envname, app);
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
        auto sval = cserve::ConfValue("cserve", "dstest", dsdefval, description, envname, app);
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
            auto sval = cserve::ConfValue("cserve", "test", defval, description, envname, app);
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
        auto sval = cserve::ConfValue("cserve", "lrtest", lrdefval, description, envname, app);
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
        auto ival = cserve::ConfValue("cserve", "itest", idefval, description, envname, app);
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
        auto ival = cserve::ConfValue("cserve", "ftest", fdefval, description, envname, app);
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
        auto ival = cserve::ConfValue("cserve", "ftest", sdefval, description, envname, app);
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
        auto dsval = cserve::ConfValue("cserve", "dstest", dsdefval, description, envname, app);
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
        auto dsval = cserve::ConfValue("cserve", "lltest", lldefval, description, envname, app);
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
        auto lrval = cserve::ConfValue("cserve", "lrtest", lrdefval, description, envname, app);
        putenv((char *) "LRTEST=POST:/hoppla:hoppla.lua;DELETE:/delme:delme.lua");
        int argc = 1;
        char const *argv[] = {"program"};
        app->parse(argc, argv);
        auto result = lrval.get_luaroutes().value();
        REQUIRE(result.size() == 2);
        REQUIRE(result[0] == cserve::LuaRoute("POST:/hoppla:hoppla.lua"));
        REQUIRE(result[1] == cserve::LuaRoute("DELETE:/delme:delme.lua"));
    }
}

TEST_CASE("Testing CserverConf class", "[CserverConf]") {
    SECTION("Command line params testing with default values") {
        cserve::CserverConf config;
        const std::string prefix{"cserve"};
        config.add_config(prefix, "config", "", "Config file");
        config.add_config(prefix, "itest", 4711, "Test integer parameter [default=4711]");
        config.add_config(prefix, "ftest", 3.1415f, "Test float parameter [default=3.1415]");
        config.add_config(prefix, "stest", "test", "Test string parameter [default=test]");
        config.add_config(prefix, "dstest", cserve::DataSize("1MB"), "Test datasize parameter [default=1MB]");
        config.add_config(prefix, "lltest", spdlog::level::level_enum::info, "Test datasize parameter [default=INFO]");
        std::vector<cserve::LuaRoute> lrdefval {
                cserve::LuaRoute("GET:/gaga:gaga.lua"),
                cserve::LuaRoute("PUT:/gugus:gugus.lua"),
        };
        config.add_config(prefix, "lrtest", lrdefval, "Test datasize parameter [default=\"GET:/gaga:gaga.lua\" \"PUT:/gugus:gugus.lua\"]");

        int argc = 1;
        const char *argv[] = { "test"};
        config.parse_cmdline_args(argc, argv);

        REQUIRE(config.get_int("itest").value() == 4711);
        REQUIRE(config.get_float("ftest").value() == 3.1415f);
        REQUIRE(config.get_string("stest").value() == std::string("test"));
        REQUIRE(config.get_datasize("dstest").value() == cserve::DataSize("1MB"));
        REQUIRE(config.get_loglevel("lltest").value() == spdlog::level::level_enum::info);
        auto result = config.get_luaroutes("lrtest").value();
        REQUIRE(result.size() == 2);
        REQUIRE(result[0] == cserve::LuaRoute("GET:/gaga:gaga.lua"));
        REQUIRE(result[1] == cserve::LuaRoute("PUT:/gugus:gugus.lua"));
    }

    SECTION("Command line params testing with cmdline options") {
        cserve::CserverConf config;
        const std::string prefix{"cserve"};
        config.add_config(prefix, "config", "", "Config file");
        config.add_config(prefix, "itest", 4711, "Test integer parameter [default=4711]");
        config.add_config(prefix, "ftest", 3.1415f, "Test float parameter [default=3.1415]");
        config.add_config(prefix, "stest", "test", "Test string parameter [default=test]");
        config.add_config(prefix, "dstest", cserve::DataSize("1MB"), "Test datasize parameter [default=1MB]");
        config.add_config(prefix, "lltest", spdlog::level::level_enum::info, "Test datasize parameter [default=INFO]");
        std::vector<cserve::LuaRoute> lrdefval {
                cserve::LuaRoute("GET:/gaga:gaga.lua"),
                cserve::LuaRoute("PUT:/gugus:gugus.lua"),
        };
        config.add_config(prefix, "lrtest", lrdefval, "Test datasize parameter [default=\"GET:/gaga:gaga.lua\" \"PUT:/gugus:gugus.lua\"]");

        int argc = 14;
        const char *argv[] = { "test",
                               "--itest", "42",
                               "--ftest", "2.71",
                               "--stest", "Waseliwas soll das?",
                               "--dstest", "2TB",
                               "--lltest", "ERR",
                               "--lrtest", "POST:/upload:upload_file.lua", "DELETE:/delete:delete.lua"};
        config.parse_cmdline_args(argc, argv);

        REQUIRE(config.get_int("itest").value() == 42);
        REQUIRE(config.get_float("ftest").value() == 2.71f);
        REQUIRE(config.get_string("stest").value() == std::string("Waseliwas soll das?"));
        REQUIRE(config.get_datasize("dstest").value() == cserve::DataSize("2TB"));
        REQUIRE(config.get_loglevel("lltest").value() == spdlog::level::level_enum::err);
        auto result = config.get_luaroutes("lrtest").value();
        REQUIRE(result.size() == 2);
        REQUIRE(result[0] == cserve::LuaRoute("POST:/upload:upload_file.lua"));
        REQUIRE(result[1] == cserve::LuaRoute("DELETE:/delete:delete.lua"));
    }

    SECTION("Command line params testing with environmant variables") {
        cserve::CserverConf config;
        const std::string prefix{"cserve"};
        config.add_config(prefix, "config", "", "Config file");
        config.add_config(prefix, "itest", 4711, "Test integer parameter [default=4711]");
        config.add_config(prefix, "ftest", 3.1415f, "Test float parameter [default=3.1415]");
        config.add_config(prefix, "stest", "test", "Test string parameter [default=test]");
        config.add_config(prefix, "dstest", cserve::DataSize("1MB"), "Test datasize parameter [default=1MB]");
        config.add_config(prefix, "lltest", spdlog::level::level_enum::info, "Test datasize parameter [default=INFO]");
        std::vector<cserve::LuaRoute> lrdefval {
                cserve::LuaRoute("GET:/gaga:gaga.lua"),
                cserve::LuaRoute("PUT:/gugus:gugus.lua"),
        };
        config.add_config(prefix, "lrtest", lrdefval, "Test datasize parameter [default=\"GET:/gaga:gaga.lua\" \"PUT:/gugus:gugus.lua\"]");

        putenv((char *) "CSERVE_ITEST=42");
        putenv((char *) "CSERVE_FTEST=2.71");
        putenv((char *) "CSERVE_STEST=Waseliwas soll das?");
        putenv((char *) "CSERVE_DSTEST=2TB");
        putenv((char *) "CSERVE_LLTEST=ERR");
        putenv((char *) "CSERVE_LRTEST=POST:/upload:upload_file.lua;DELETE:/delete:delete.lua");
        int argc = 1;
        const char *argv[] = { "test"};
        config.parse_cmdline_args(argc, argv);

        REQUIRE(config.get_int("itest").value() == 42);
        REQUIRE(config.get_float("ftest").value() == 2.71f);
        REQUIRE(config.get_string("stest").value() == std::string("Waseliwas soll das?"));
        REQUIRE(config.get_datasize("dstest").value() == cserve::DataSize("2TB"));
        REQUIRE(config.get_loglevel("lltest").value() == spdlog::level::level_enum::err);
        auto result = config.get_luaroutes("lrtest").value();
        REQUIRE(result.size() == 2);
        REQUIRE(result[0] == cserve::LuaRoute("POST:/upload:upload_file.lua"));
        REQUIRE(result[1] == cserve::LuaRoute("DELETE:/delete:delete.lua"));
        unsetenv((char *) "CSERVE_ITEST");
        unsetenv((char *) "CSERVE_FTEST");
        unsetenv((char *) "CSERVE_STEST");
        unsetenv((char *) "CSERVE_DSTEST");
        unsetenv((char *) "CSERVE_LLTEST");
        unsetenv((char *) "CSERVE_LRTEST");
    }

    SECTION("Command line params testing with lua config script options") {
        cserve::CserverConf config;
        const std::string prefix{"cserve"};
        config.add_config(prefix, "config", "", "Config file");
        config.add_config(prefix, "itest", 4711, "Test integer parameter [default=4711]");
        config.add_config(prefix, "ftest", 3.1415f, "Test float parameter [default=3.1415]");
        config.add_config(prefix, "stest", "test", "Test string parameter [default=test]");
        config.add_config(prefix, "dstest", cserve::DataSize("1MB"), "Test datasize parameter [default=1MB]");
        config.add_config(prefix, "lltest", spdlog::level::level_enum::info, "Test datasize parameter [default=INFO]");
        std::vector<cserve::LuaRoute> lrdefval {
                cserve::LuaRoute("GET:/gaga:gaga.lua"),
                cserve::LuaRoute("PUT:/gugus:gugus.lua"),
        };
        config.add_config(prefix, "lrtest", lrdefval, "Test datasize parameter [default=\"GET:/gaga:gaga.lua\" \"PUT:/gugus:gugus.lua\"]");

        int argc = 3;
        const char *argv[] = { "test",
                               "--config", "./testdata/test-config.lua"};
        config.parse_cmdline_args(argc, argv);

        REQUIRE(config.get_int("itest").value() == 1234);
        REQUIRE(config.get_float("ftest").value() == 0.123f);
        REQUIRE(config.get_string("stest").value() == std::string("from lua"));
        REQUIRE(config.get_datasize("dstest").value() == cserve::DataSize("8KB"));
        REQUIRE(config.get_loglevel("lltest").value() == spdlog::level::level_enum::off);
        auto result = config.get_luaroutes("lrtest").value();
        REQUIRE(result.size() == 1);
        REQUIRE(result[0] == cserve::LuaRoute("GET:/lualua:lualua.lua"));
    }
}

TEST_CASE("Testing CserverConf class", "[LuaGlobals]") {
    cserve::Connection conn;
    lua_State *L = luaL_newstate();
    cserve::CserverConf config("config");
    const std::string prefix{"cserve"};
    config.add_config(prefix, "config", "", "Config file");
    config.add_config(prefix, "itest", 4711, "Test integer parameter [default=4711]");
    config.add_config(prefix, "ftest", 3.1415f, "Test float parameter [default=3.1415]");
    config.add_config(prefix, "stest", "test", "Test string parameter [default=test]");
    config.add_config(prefix, "dstest", cserve::DataSize("1MB"), "Test datasize parameter [default=1MB]");
    config.add_config(prefix, "lltest", spdlog::level::level_enum::info, "Test datasize parameter [default=INFO]");
    std::vector<cserve::LuaRoute> lrdefval{
            cserve::LuaRoute("GET:/gaga:gaga.lua"),
            cserve::LuaRoute("PUT:/gugus:gugus.lua"),
    };
    config.add_config(prefix, "lrtest", lrdefval,
                      "Test datasize parameter [default=\"GET:/gaga:gaga.lua\" \"PUT:/gugus:gugus.lua\"]");

    cserverConfGlobals(L, conn, &config);

    REQUIRE(lua_getglobal(L, "config") == LUA_TTABLE); // config

    REQUIRE(lua_getfield(L, -1, "itest") == LUA_TNUMBER); // config – itest-value
    REQUIRE(lua_tointeger(L, -1) == 4711);
    lua_pop(L, -2); // config

    REQUIRE(lua_getfield(L, -1, "ftest") == LUA_TNUMBER); // config – ftest-value
    REQUIRE(lua_tonumber(L, -1) == 3.1415f);
    lua_pop(L, -2); // config

    REQUIRE(lua_getfield(L, -1, "stest") == LUA_TSTRING); // config - stest
    REQUIRE(strcmp(lua_tostring(L, -1), "test") == 0);
    lua_pop(L, -2); // config

    REQUIRE(lua_getfield(L, -1, "dstest") == LUA_TSTRING); // config - dstest
    REQUIRE(strcmp(lua_tostring(L, -1), "1MB") == 0);
    lua_pop(L, -2); // config

    REQUIRE(lua_getfield(L, -1, "lltest") == LUA_TSTRING); // config - lltest
    REQUIRE(strcmp(lua_tostring(L, -1), "INFO") == 0);
    lua_pop(L, -2); // config

    REQUIRE(lua_getfield(L, -1, "lrtest") == LUA_TTABLE);  // config - lrtest
    REQUIRE(lua_rawgeti(L, -1, 0) == LUA_TTABLE);          // config - lrtest - route1_table
    REQUIRE(lua_getfield(L, -1, "method") == LUA_TSTRING); // config - lrtest - route1_table - method_value
    REQUIRE(strcmp(lua_tostring(L, -1), "GET") == 0);
    lua_pop(L, 1);                                              // config - lrtest - route1_table
    REQUIRE(lua_getfield(L, -1, "_route") == LUA_TSTRING); // config - lrtest - route1_table - route_value
    REQUIRE(strcmp(lua_tostring(L, -1), "/gaga") == 0);
    lua_pop(L, 1);  // config - lrtest
    REQUIRE(lua_getfield(L, -1, "script") == LUA_TSTRING); // config - lrtest - route1 - script_value
    REQUIRE(strcmp(lua_tostring(L, -1), "gaga.lua") == 0);
    lua_pop(L, 2);  // config - lrtest

    REQUIRE(lua_rawgeti(L, -1, 1) == LUA_TTABLE);          // config - lrtest - route2_table
    REQUIRE(lua_getfield(L, -1, "method") == LUA_TSTRING); // config - lrtest - route2_table - method_value
    REQUIRE(strcmp(lua_tostring(L, -1), "PUT") == 0);
    lua_pop(L, 1);                                              // config - lrtest - route2_table
    REQUIRE(lua_getfield(L, -1, "_route") == LUA_TSTRING); // config - lrtest - route2_table - route_value
    REQUIRE(strcmp(lua_tostring(L, -1), "/gugus") == 0);
    lua_pop(L, 1);  // config - lrtest
    REQUIRE(lua_getfield(L, -1, "script") == LUA_TSTRING); // config - lrtest - route2 - script_value
    REQUIRE(strcmp(lua_tostring(L, -1), "gugus.lua") == 0);
    lua_pop(L, 4);

    lua_close(L);
}