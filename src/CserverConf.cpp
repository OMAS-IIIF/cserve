//
// Created by Lukas Rosenthaler on 28.06.21.
//

#include <thread>
#include <utility>

#include "CLI11.hpp"

#include "Global.h"
#include "LuaServer.h"
#include "CserverConf.h"
#include "Connection.h"

static const char file_[] = __FILE__;


void cserverConfGlobals(lua_State *L, cserve::Connection &conn, void *user_data) {
    auto *conf = (CserverConf *) user_data;

    lua_createtable(L, 0, 15); // table1

    const auto values = conf->get_values();
    for (auto const& [name, val] : values) {
        lua_pushstring(L, name.c_str()); // table1 - "index_L1"
        auto vtype = val->get_type();
        switch (vtype) {
            case cserve::ConfValue::INTEGER:
                lua_pushinteger(L, val->get_int().value_or(-1)); // "table1 - index_L1 - value_L1"
                break;
            case cserve::ConfValue::FLOAT:
                lua_pushnumber(L, val->get_float().value_or(0.0)); // "table1 - index_L1 - value_L1"
                break;
            case cserve::ConfValue::STRING:
                lua_pushstring(L, val->get_string().value_or("").c_str()); // "table1 - index_L1 - value_L1"
                break;
            case cserve::ConfValue::DATASIZE:
                lua_pushstring(L, val->get_datasize().value_or(cserve::DataSize()).as_string().c_str()); // "table1 - index_L1 - value_L1"
                break;
            case cserve::ConfValue::LOGLEVEL:
                lua_pushstring(L, val->get_loglevel_as_string().value_or("OFF").c_str()); // "table1 - index_L1 - value_L1"
                break;
            case cserve::ConfValue::LUAROUTES:
                std::vector<cserve::LuaRoute> routes = val->get_luaroutes().value();
                lua_createtable(L, routes.size(), 0);  // "table1 - index_L1 - table2"
                int index = 0;
                for (auto& route: routes) {
                    lua_createtable(L, 0, 3); // "table1 - index_L1 - table2 - table3"
                    lua_pushstring(L, "method"); // "table1 - index_L1 - table2 - table3 - "method"
                    lua_pushstring(L, route.method_as_string().c_str()); // "table1 - index_L1 - table2 - table3 - "method" - value"
                    lua_settable(L, -3); // "table1 - index_L1 - table2 - table3"
                    lua_pushstring(L, "route"); // "table1 - index_L1 - table2 - table3 - "route"
                    lua_pushstring(L, route.route.c_str()); // "table1 - index_L1 - table2 - table3 - "method" - value"
                    lua_settable(L, -3); // "table1 - index_L1 - table2 - table3"
                    lua_pushstring(L, "script"); // "table1 - index_L1 - table2 - table3 - "route"
                    lua_pushstring(L, route.script.c_str()); // "table1 - index_L1 - table2 - table3 - "method" - value"
                    lua_settable(L, -3); // "table1 - index_L1 - table2 - table3"
                    lua_rawseti(L, -2, index++); // "table1 - index_L1 - table2"
                }
                break;
        }
        lua_rawset(L, -3); // table1
    }

    lua_setglobal(L, "config");
}

std::optional<int> CserverConf::get_int(const std::string &name) {
    try {
        auto val = _values.at(name);
        return val->get_int();
    }
    catch (std::out_of_range &err) {
        return {};
    }
}

std::optional<float> CserverConf::get_float(const std::string &name) {
    try {
        auto val = _values.at(name);
        return val->get_float();
    }
    catch (std::out_of_range &err) {
        return {};
    }
}

std::optional<std::string> CserverConf::get_string(const std::string &name) {
    try {
        auto val = _values.at(name);
        return val->get_string();
    }
    catch (std::out_of_range &err) {
        return {};
    }
}

std::optional<cserve::DataSize> CserverConf::get_datasize(const std::string &name) {
    try {
        auto val = _values.at(name);
        return val->get_datasize();
    }
    catch (std::out_of_range &err) {
        return {};
    }
}

CserverConf::CserverConf() {
    //
    // first we set the hardcoded defaults that are used if nothing else has been defined
    //
    _cserverOpts = std::make_shared<CLI::App>("cserver is a small C++ based webserver with Lua integration.", "cserver");
    _serverconf_ok = 0;
}

void CserverConf::add_config(std::string name, int defaultval, std::string description) {
    std::string optionname = "--" + name;
    std::string envname = "CSERVER_" + cserve::strtoupper(name);
    _values.emplace(name, std::make_shared<cserve::ConfValue>(optionname, defaultval, std::move(description), envname, _cserverOpts));
}

void CserverConf::add_config(std::string name, float defaultval, std::string description) {
    std::string optionname = "--" + name;
    std::string envname = "CSERVER_" + cserve::strtoupper(name);
    _values.emplace(name, std::make_shared<cserve::ConfValue>(optionname, defaultval, std::move(description), envname, _cserverOpts));
}

void CserverConf::add_config(std::string name, const char *defaultval, std::string description) {
    std::string optionname = "--" + name;
    std::string envname = "CSERVER_" + cserve::strtoupper(name);
    _values.emplace(name, std::make_shared<cserve::ConfValue>(optionname, defaultval, std::move(description), envname, _cserverOpts));
}

void CserverConf::add_config(std::string name, std::string defaultval, std::string description) {
    std::string optionname = "--" + name;
    std::string envname = "CSERVER_" + cserve::strtoupper(name);
    _values.emplace(name, std::make_shared<cserve::ConfValue>(optionname, std::move(defaultval), std::move(description), envname, _cserverOpts));
}

void CserverConf::add_config(std::string name, const cserve::DataSize &defaultval, std::string description) {
    std::string optionname = "--" + name;
    std::string envname = "CSERVER_" + cserve::strtoupper(name);
    _values.emplace(name, std::make_shared<cserve::ConfValue>(optionname, defaultval, std::move(description), envname, _cserverOpts));
}

void CserverConf::add_config(std::string name, spdlog::level::level_enum defaultval, std::string description) {
    std::string optionname = "--" + name;
    std::string envname = "CSERVER_" + cserve::strtoupper(name);
    _values.emplace(name, std::make_shared<cserve::ConfValue>(optionname, defaultval, std::move(description), envname, _cserverOpts));
}

void CserverConf::add_config(std::string name, std::vector<cserve::LuaRoute> defaultval, std::string description) {
    std::string optionname = "--" + name;
    std::string envname = "CSERVER_" + cserve::strtoupper(name);
    _values.emplace(name, std::make_shared<cserve::ConfValue>(optionname, std::move(defaultval), std::move(description), envname, _cserverOpts));
}

void CserverConf::parse_cmdline_args(int argc, char *argv[]) {

    try {
        _cserverOpts->parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        _serverconf_ok = _cserverOpts->exit(e);
        return;
    }

    for (auto const& [name, val] : _values) {
        std::cerr << "2) **************************NAME=" << name << " VALUE=" << val << std::endl;
    }

    if (!_cserverOpts->get_option("--config")->empty()) {
        cserve::LuaServer luacfg = cserve::LuaServer(_values["config"]->get_string().value());
        typedef std::variant<int, float, std::string, cserve::DataSize, spdlog::level::level_enum, std::vector<cserve::LuaRoute>> UnionDataType;
        std::unordered_map<std::string, UnionDataType> valmap;

        for (auto const& [name, val] : _values) {
            auto vtype = val->get_type();
            switch (vtype) {
                case cserve::ConfValue::INTEGER:
                    valmap.emplace(name, luacfg.configInteger("cserve", name, val->get_int().value()));
                    break;
                case cserve::ConfValue::FLOAT:
                    valmap.emplace(name, luacfg.configFloat("cserve", name, val->get_float().value()));
                    break;
                case cserve::ConfValue::STRING:
                    valmap.emplace(name, luacfg.configString("cserve", name, val->get_string().value()));
                    break;
                case cserve::ConfValue::DATASIZE:
                    valmap.emplace(name, luacfg.configString("cserve", name, val->get_datasize().value().as_string()));
                    break;
                case cserve::ConfValue::LOGLEVEL:
                    valmap.emplace(name, luacfg.configLoglevel("cserve", name, val->get_loglevel().value()));
                    break;
                case cserve::ConfValue::LUAROUTES:
                    valmap.emplace(name, luacfg.configRoute(name));
                    break;
            }
        }
        for (auto const& [name, val] : _values) {
            if (_cserverOpts->get_option(val->get_optionname())->empty()) {
                auto vtype = val->get_type();
                switch (vtype) {
                    case cserve::ConfValue::INTEGER:
                        _values[name]->set_value(std::get<int>(valmap[name]));
                        break;
                    case cserve::ConfValue::FLOAT:
                        _values[name]->set_value(std::get<float>(valmap[name]));
                        break;
                    case cserve::ConfValue::STRING:
                        _values[name]->set_value(std::get<std::string>(valmap[name]));
                        break;
                    case cserve::ConfValue::DATASIZE:
                        _values[name]->set_value(std::get<cserve::DataSize>(valmap[name]));
                        break;
                    case cserve::ConfValue::LOGLEVEL:
                        _values[name]->set_value(std::get<spdlog::level::level_enum>(valmap[name]));
                        break;
                    case cserve::ConfValue::LUAROUTES:
                        _values[name]->set_value(std::get<std::vector<cserve::LuaRoute>>(valmap[name]));
                        break;
                }
            }
        }
    }
}