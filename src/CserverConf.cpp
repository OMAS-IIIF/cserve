//
// Created by Lukas Rosenthaler on 28.06.21.
//

#include <thread>

#include "CLI11.hpp"

#include "Global.h"
#include "LuaServer.h"
#include "CserverConf.h"
#include "Connection.h"
#include "LuaConfig.h"

static const char file_[] = __FILE__;


void cserverConfGlobals(lua_State *L, cserve::Connection &conn, void *user_data) {
    auto *conf = (CserverConf *) user_data;

    lua_createtable(L, 0, 15); // table1

    const auto values = conf->get_values();
    for (auto const& [name, val] : values) {
        lua_pushstring(L, name.c_str()); // table1 - "index_L1"
        auto vtype = val.get_type();
        switch (vtype) {
            case cserve::ConfValue::INTEGER:
                lua_pushinteger(L, val.get_int().value_or(-1));
                break;
            case cserve::ConfValue::FLOAT:
                lua_pushnumber(L, val.get_float().value_or(0.0));
                break;
            case cserve::ConfValue::STRING:
                lua_pushstring(L, val.get_string().value_or("").c_str());
                break;
            case cserve::ConfValue::DATASIZE:
                lua_pushstring(L, val.get_datasize().value_or(cserve::DataSize()).as_string().c_str());
                break;
            case cserve::ConfValue::LOGLEVEL:
                lua_pushstring(L, val.get_loglevel().value_or(LogLevel()).as_string().c_str());
                break;
            case cserve::ConfValue::LUAROUTES:
                break;
        }
        lua_rawset(L, -3); // table1
    }

    lua_setglobal(L, "config");
}

std::optional<int> CserverConf::get_int(const std::string &name) {
    try {
        auto val = _values.at(name);
        return val.get_int();
    }
    catch (std::out_of_range &err) {
        return {};
    }
}

std::optional<float> CserverConf::get_float(const std::string &name) {
    try {
        auto val = _values.at(name);
        return val.get_float();
    }
    catch (std::out_of_range &err) {
        return {};
    }
}

std::optional<std::string> CserverConf::get_string(const std::string &name) {
    try {
        auto val = _values.at(name);
        return val.get_string();
    }
    catch (std::out_of_range &err) {
        return {};
    }
}

CserverConf::CserverConf(int argc, char *argv[]) {
    //
    // first we set the hardcoded defaults that are used if nothing else has been defined
    //
    _cserverOpts = std::make_shared<CLI::App>("cserver is a small C++ based webserver with Lua integration.", "cserver");
    _serverconf_ok = 0;

    _values.emplace("config", cserve::ConfValue("-c,--config", "./config", "Configuration file for web server.", "CSERVER_CONFIGFILE", _cserverOpts));
    _values.emplace("handlerdir", cserve::ConfValue("-h,--handlerdir", "./handler", "", "CSERVER_HANLDERDIR", _cserverOpts));
    _values.emplace("userid", cserve::ConfValue("-u,--userid", "", "Username to use to run cserver.", "CSERVER_USERID", _cserverOpts));
    _values.emplace("port", cserve::ConfValue("-p,--port", 8080, "", "CSERVER_PORT", _cserverOpts));
    _values.emplace("ssl_port", cserve::ConfValue("--sslport", 8443, "HTTP port to use for cserver.", "CSERVER_SSLPORT", _cserverOpts));
    _values.emplace("ssl_certificate", cserve::ConfValue("-sslcert", "./certificate/certificate.pem", "Path to SSL certificate.", "CSERVER_SSLCERTIFICATE", _cserverOpts));
    _values.emplace("ssl_key", cserve::ConfValue("--sslkey", "./certificate/key.pem", "Path to the SSL key file.", "CSERVER_SSLKEY", _cserverOpts));
    _values.emplace("jwt_secret", cserve::ConfValue("--jwtkey","UP4014, the biggest steam engine", "The secret for generating JWT's (JSON Web Tokens) (exactly 42 characters).", "CSERVER_JWTKEY", _cserverOpts));
    _values.emplace("nthreads", cserve::ConfValue("-t,--nthreads", static_cast<int>(std::thread::hardware_concurrency()), "Number of worker threads to be used by cserver\"", "CSERVER_NTHREADS", _cserverOpts));
    _values.emplace("docroot", cserve::ConfValue("--docroot", "./docroot", "Path to document root for file server.", "CSERVER_DOCROOT", _cserverOpts)); // TODO: Move to file server plugin
    _values.emplace("wwwroute", cserve::ConfValue("--wwwroute", "/", "Route root for file server.", "CSERVER_WWW_ROUTE", _cserverOpts)); // TODO: Move to file server plugin
    _values.emplace("tmpdir", cserve::ConfValue("--tmpdir", "./tmp", "Path to the temporary directory (e.g. for uploads etc.).", "CSERVER_TMPDIR", _cserverOpts));
    _values.emplace("scriptdir", cserve::ConfValue("--scriptdir", "./scripts", "Path to directory containing Lua scripts to implement routes.", "CSERVER_SCRIPTDIR", _cserverOpts)); // Todo: Move to script handler plugin
    _values.emplace("routes", cserve::ConfValue("--routes", std::vector<cserve::LuaRoute>{}, "Lua routes in the form \"<http-type>:<route>:<script>\"", "CSERVER_LUAROUTES", _cserverOpts));  // Todo: Move to script handler plugin
    _values.emplace("keep_alive", cserve::ConfValue("--keepalive", 5, "Number of seconds for the keep-alive option of HTTP 1.1.", "CSERVER_KEEPALIVE", _cserverOpts));
    _values.emplace("max_post_size", cserve::ConfValue("--maxpost", cserve::DataSize("50MB"), "A string indicating the maximal size of a POST request, e.g. '100M'.", "CSERVER_MAXPOSTSIZE", _cserverOpts)); // 50MB
    _values.emplace("initscript", cserve::ConfValue("--initscript", "", "Path to LUA init script.", "CSERVER_INITSCRIPT", _cserverOpts));
    _values.emplace("logfile", cserve::ConfValue("--logfile", "./cserver.log", "Name of the logfile.", "CSERVE_LOGFILE", _cserverOpts));
    _values.emplace("loglevel", cserve::ConfValue("--loglevel", spdlog::level::debug, "Logging level Value can be: 'TRACE', 'DEBUG', 'INFO', 'WARN', 'ERR', 'CRITICAL', 'OFF'.", "CSERVER_LOGLEVEL", _cserverOpts));

    try {
        _cserverOpts->parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        _serverconf_ok = _cserverOpts->exit(e);
        return;
    }

    if (!_cserverOpts->get_option("--config")->empty()) {
        cserve::LuaServer luacfg = cserve::LuaServer(_values["config"].get_string().value());
        typedef std::variant<int, float, std::string, cserve::DataSize, spdlog::level::level_enum, std::vector<cserve::LuaRoute>> UnionDataType;
        std::unordered_map<std::string, UnionDataType> valmap;

        for (auto const& [name, val] : _values) {
            auto vtype = val.get_type();
            switch (vtype) {
                case cserve::ConfValue::INTEGER:
                    valmap.emplace(name, luacfg.configInteger("cserve", name, val.get_int().value()));
                    break;
                case cserve::ConfValue::FLOAT:
                    valmap.emplace(name, luacfg.configFloat("cserve", name, val.get_float().value()));
                    break;
                case cserve::ConfValue::STRING:
                    valmap.emplace(name, luacfg.configString("cserve", name, val.get_string().value()));
                    break;
                case cserve::ConfValue::DATASIZE:
                    valmap.emplace(name, luacfg.configString("cserve", name, val.get_datasize().value().as_string()));
                    break;
                case cserve::ConfValue::LOGLEVEL:
                    valmap.emplace(name, luacfg.configLoglevel("cserve", name, val.get_loglevel().value()));
                    break;
                case cserve::ConfValue::LUAROUTES:
                     valmap.emplace(name, luacfg.configRoute(name));
                    break;
            }
        }
        for (auto const& [name, val] : _values) {
            if (_cserverOpts->get_option(val.get_optionname())->empty()) {
                auto vtype = val.get_type();
                switch (vtype) {
                    case cserve::ConfValue::INTEGER:
                        _values[name].set_value(std::get<int>(valmap[name]));
                        break;
                    case cserve::ConfValue::FLOAT:
                        _values[name].set_value(std::get<float>(valmap[name]));
                        break;
                    case cserve::ConfValue::STRING:
                        _values[name].set_value(std::get<std::string>(valmap[name]));
                        break;
                    case cserve::ConfValue::DATASIZE:
                        _values[name].set_value(std::get<cserve::DataSize>(valmap[name]));
                        break;
                    case cserve::ConfValue::LOGLEVEL:
                        _values[name].set_value(std::get<spdlog::level::level_enum>(valmap[name]));
                        break;
                    case cserve::ConfValue::LUAROUTES:
                        _values[name].set_value(std::get<std::vector<cserve::LuaRoute>>(valmap[name]));
                        break;
                }
            }
        }
    }


}

void CserverConf::parse_cmdline_args(int argc, char *argv[]) {

}