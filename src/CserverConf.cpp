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
    const auto values = conf->get_values();
    lua_newtable(L); // table1
//    lua_pushstring(L, "userid");
//    lua_pushinteger(L, 1111);
//    lua_rawset(L, -3); // table1

    lua_pushstring(L, "port");
    lua_pushinteger(L, 2222);
    lua_rawset(L, -3);
    lua_setglobal(L, "config");
    return;
    for (auto const& [name, val] : values) {
        if (name == "userid") continue;
        if (name == "docroot") continue;
        if (name == "loglevel") continue;
        if (name == "nthreads") continue;
        //if (name == "port") continue;
        if (name == "config") continue;
        if (name == "jwtkey") continue;
        if (name == "initscript") continue;
        if (name == "sslkey") continue;
        if (name == "sslport") continue;
        if (name == "sslcert") continue;
        // if (name == "keepalive") continue;
        if (name == "tmpdir") continue;
        if (name == "scriptdir") continue;
        if (name == "logfile") continue;
        if (name == "routes") continue;
        if (name == "maxpost") continue;
        if (name == "handlerdir") continue;
        if (name == "docroot") continue;
        if (name == "wwwroute") continue;

        lua_pushstring(L, name.c_str()); // table1 - "index_L1"
        auto vtype = val->get_type();
        switch (vtype) {
            case cserve::ConfValue::INTEGER:
                lua_pushinteger(L, val->get_int().value_or(-1)); // "table1 - index_L1 - value_L1"
                std::cerr << "   ++++++++INTEGER> " << name << "->" << val->get_int().value_or(-1) <<  std::endl;
                break;
            case cserve::ConfValue::FLOAT:
                lua_pushnumber(L, val->get_float().value_or(0.0)); // "table1 - index_L1 - value_L1"
                std::cerr << "   ++++++++FLOAT> " << name << "->"  << val->get_float().value_or(0.0) <<  std::endl;
                break;
            case cserve::ConfValue::STRING:
                //lua_pushstring(L, val->get_string().value_or("").c_str()); // "table1 - index_L1 - value_L1"
                lua_pushstring(L, "...A STRING..."); // "table1 - index_L1 - value_L1"
                std::cerr << "   ++++++++STRING> " << name << "->"  << val->get_string().value_or("") <<  std::endl;
                break;
            case cserve::ConfValue::DATASIZE:
                lua_pushstring(L, val->get_datasize().value_or(cserve::DataSize()).as_string().c_str()); // "table1 - index_L1 - value_L1"
                std::cerr << "   ++++++++DATASIZE> " << name << "->"  << val->get_datasize().value_or(cserve::DataSize()).as_string() <<  std::endl;
                break;
            case cserve::ConfValue::LOGLEVEL:
                lua_pushstring(L, val->get_loglevel_as_string().value_or("OFF").c_str()); // "table1 - index_L1 - value_L1"
                std::cerr << "   ++++++++LOGLEVEL> " << name << "->"  << val->get_loglevel_as_string().value_or("OFF") <<  std::endl;
                break;
            case cserve::ConfValue::LUAROUTES:
                lua_pushstring(L, "GAGAGAGAGAGAA");
                std::cerr << "   ++++++++LUAROUTES> " << name << "->"  << "GAGAGAGAGAGAA" <<  std::endl;
                break;
                /*
                std::vector<cserve::LuaRoute> routes = val->get_luaroutes().value();
                lua_createtable(L, routes.size(), 0);  // "table1 - index_L1 - table2"
                int index = 0;
                for (auto& route: routes) {
                    lua_createtable(L, 0, 3); // "table1 - index_L1 - table2 - table3"
                    lua_pushstring(L, "method"); // "table1 - index_L1 - table2 - table3 - "method"
                    lua_pushstring(L, route.method_as_string().c_str()); // "table1 - index_L1 - table2 - table3 - "method" - value"
                    std::cerr << "   ++++++++method> " << route.method_as_string() <<  std::endl;
                    lua_settable(L, -3); // "table1 - index_L1 - table2 - table3"
                    lua_pushstring(L, "route"); // "table1 - index_L1 - table2 - table3 - "route"
                    lua_pushstring(L, route.route.c_str()); // "table1 - index_L1 - table2 - table3 - "method" - value"
                    std::cerr << "   ++++++++route> " << route.route <<  std::endl;
                    lua_settable(L, -3); // "table1 - index_L1 - table2 - table3"
                    lua_pushstring(L, "script"); // "table1 - index_L1 - table2 - table3 - "route"
                    lua_pushstring(L, route.script.c_str()); // "table1 - index_L1 - table2 - table3 - "method" - value"
                    std::cerr << "   ++++++++script> " << route.script <<  std::endl;
                    lua_settable(L, -3); // "table1 - index_L1 - table2 - table3"
                    lua_rawseti(L, -2, index++); // "table1 - index_L1 - table2"
                }
                break;
                 */
            default:
                std::cerr << "--------------->" << name << std::endl;
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

std::optional<std::vector<cserve::LuaRoute>> CserverConf::get_luaroutes(const std::string &name) {
    try {
        auto val = _values.at(name);
        return val->get_luaroutes();
    }
    catch (std::out_of_range &err) {
        return {};
    }
}

std::optional<spdlog::level::level_enum> CserverConf::get_loglevel(const std::string &name) {
    try {
        auto val = _values.at(name);
        return val->get_loglevel();
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
    _values[name] =  std::make_shared<cserve::ConfValue>(optionname, defaultval, std::move(description), envname, _cserverOpts);
}

void CserverConf::add_config(std::string name, float defaultval, std::string description) {
    std::string optionname = "--" + name;
    std::string envname = "CSERVER_" + cserve::strtoupper(name);
    _values[name] = std::make_shared<cserve::ConfValue>(optionname, defaultval, std::move(description), envname, _cserverOpts);
}

void CserverConf::add_config(std::string name, const char *defaultval, std::string description) {
    std::string optionname = "--" + name;
    std::string envname = "CSERVER_" + cserve::strtoupper(name);
    _values[name] = std::make_shared<cserve::ConfValue>(optionname, std::string(defaultval), std::move(description), envname, _cserverOpts);
}

void CserverConf::add_config(std::string name, std::string defaultval, std::string description) {
    std::string optionname = "--" + name;
    std::string envname = "CSERVER_" + cserve::strtoupper(name);
    _values[name] =  std::make_shared<cserve::ConfValue>(optionname, std::move(defaultval), std::move(description), envname, _cserverOpts);
}

void CserverConf::add_config(std::string name, const cserve::DataSize &defaultval, std::string description) {
    std::string optionname = "--" + name;
    std::string envname = "CSERVER_" + cserve::strtoupper(name);
    _values[name] = std::make_shared<cserve::ConfValue>(optionname, defaultval, std::move(description), envname, _cserverOpts);
}

void CserverConf::add_config(std::string name, spdlog::level::level_enum defaultval, std::string description) {
    std::string optionname = "--" + name;
    std::string envname = "CSERVER_" + cserve::strtoupper(name);
    _values[name] = std::make_shared<cserve::ConfValue>(optionname, defaultval, std::move(description), envname, _cserverOpts);
}

void CserverConf::add_config(std::string name, std::vector<cserve::LuaRoute> defaultval, std::string description) {
    std::string optionname = "--" + name;
    std::string envname = "CSERVER_" + cserve::strtoupper(name);
    _values[name] = std::make_shared<cserve::ConfValue>(optionname, std::move(defaultval), std::move(description), envname, _cserverOpts);
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