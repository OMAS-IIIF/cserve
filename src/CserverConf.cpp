//
// Created by Lukas Rosenthaler on 28.06.21.
//

#include <thread>

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
    _values.emplace("filehandler_route", cserve::ConfValue("--filehandler_route", "/", "Route root for file server.", "CSERVER_FILEHANDLER_ROUTE", _cserverOpts)); // TODO: Move to file server plugin
    _values.emplace("tmpdir", cserve::ConfValue("--tmpdir", "./tmp", "Path to the temporary directory (e.g. for uploads etc.).", "CSERVER_TMPDIR", _cserverOpts));
    _values.emplace("scriptdir", cserve::ConfValue("--scriptdir", "./scripts", "Path to directory containing Lua scripts to implement routes.", "CSERVER_SCRIPTDIR", _cserverOpts)); // Todo: Move to script handler plugin
    _values.emplace("luaroutes", cserve::ConfValue("--routes", std::vector<std::string>(), "Lua routes in the form \"<http-type>:<route>:<script>\"", "CSERVER_LUAROUTES", _cserverOpts));  // Todo: Move to script handler plugin
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
        for (auto const& [name, val] : _values) {
            auto vtype = val.get_type();
            switch (vtype) {
                case cserve::ConfValue::INTEGER:
                    luacfg.configInteger("cserve", name, val.get_int().value());
                    break;
                case cserve::ConfValue::FLOAT:
                    luacfg.configFloat("cserve", name, val.get_float().value());
                    break;
                case cserve::ConfValue::STRING:
                    luacfg.configString("cserve", name, val.get_string().value());
                    break;
                case cserve::ConfValue::DATASIZE:
                    luacfg.configString("cserve", name, val.get_datasize().value().as_string());
                    break;
                case cserve::ConfValue::LOGLEVEL:
                    std::unordered_map<spdlog::level::level_enum,std::string> logNamesMap = {
                            {spdlog::level::trace, "TRACE"},
                            {spdlog::level::debug, "DEBUG"},
                            {spdlog::level::info, "INFO"},
                            {spdlog::level::warn, "WARN",},
                            {spdlog::level::err, "ERR"},
                            {spdlog::level::critical, "CRITICAL"},
                            {spdlog::level::off, "OFF"}
                    };
                    std::string loglevelstr = logNamesMap[val.get_loglevel()];
                    luacfg.configString("cserve", name, loglevelstr);
                    break;
                case cserve::ConfValue::LUAROUTES:
                    break;
            }
            lua_rawset(L, -3); // table1
        }



        try {
            // cserve::Server::logger()->info("Reading configuration from '{}'.", optConfigfile);
            cserve::LuaServer luacfg = cserve::LuaServer(_values["config"].get_string().value());

            //_handlerdir = luacfg.configString("cserve", "handlerdir", _handlerdir);
            _userid = luacfg.configString("cserve", "userid", _userid);
            _port = luacfg.configInteger("cserve", "port", _port);

            _ssl_port = luacfg.configInteger("cserve", "ssl_port", _ssl_port);
            _ssl_certificate = luacfg.configString("cserve", "sslcert", _ssl_certificate);
            _ssl_key = luacfg.configString("cserve", "sslkey", _ssl_key);
            _jwt_secret = luacfg.configString("cserve", "jwt_secret", _jwt_secret);
            _docroot = luacfg.configString("cserve", "docroot", _docroot);
            _filehandler_route = luacfg.configString("cserve", "filehandler_route", _filehandler_route);
            _tmpdir = luacfg.configString("cserve", "tmpdir", _tmpdir);
            _scriptdir = luacfg.configString("cserve", "scriptdir", _scriptdir);
            _nthreads = luacfg.configInteger("cserve", "nthreads", static_cast<int>(_nthreads));
            _keep_alive = luacfg.configInteger("cserve", "keep_alive", _keep_alive);
            _max_post_size = luacfg.configInteger("cserve", "max_post_size", static_cast<int>(_max_post_size));
            _initscript = luacfg.configString("cserve", "initscript", _initscript);
            _logfile = luacfg.configString("cserve", "logfile", _logfile);

            std::string loglevelstr;
            for (const auto &ll: logLevelMap) { // convert spdlog::level::level_enum to std::string
                if (ll.second == _loglevel) {
                    loglevelstr = ll.first;
                    break;
                }
            }
            loglevelstr = luacfg.configString("cserve", "loglevel", loglevelstr);
            for (const auto &ll: logLevelMap) { // convert std::string to spdlog::level::level_enum
                if (ll.first == loglevelstr) {
                    _loglevel = ll.second;
                    break;
                }
            }

            _routes = luacfg.configRoute("routes");
        } catch (cserve::Error &err) {
            std::cerr << err << std::endl;
        }
    }

    //
    // now merge config file, commandline and environment variable parameters
    // config file is overwritten by environment variable is overwritten by command line param
    //
    if (!_cserverOpts.get_option("--handlerdir")->empty()) _handlerdir = optHandlerdir;
    if (!_cserverOpts.get_option("--userid")->empty()) _userid = optUserid;
    if (!_cserverOpts.get_option("--port")->empty()) _port = optServerport;
    if (!_cserverOpts.get_option("--sslport")->empty()) _ssl_port = optSSLport;
    if (!_cserverOpts.get_option("--sslcert")->empty()) _ssl_certificate = optSSLCertificatePath;
    if (!_cserverOpts.get_option("--sslkey")->empty()) _ssl_key = optSSLKeyPath;
    if (!_cserverOpts.get_option("--jwtkey")->empty()) _jwt_secret = optJWTKey;
    if (!_cserverOpts.get_option("--docroot")->empty()) _docroot = optDocroot;
    if (!_cserverOpts.get_option("--filehandler_route")->empty()) _filehandler_route = optFilehandlerRoute;
    if (!_cserverOpts.get_option("--tmpdir")->empty()) _tmpdir = optTmpdir;
    if (!_cserverOpts.get_option("--scriptdir")->empty()) _scriptdir = optScriptDir;
    if (!_cserverOpts.get_option("--nthreads")->empty()) _nthreads = optNThreads;
    if (!_cserverOpts.get_option("--keepalive")->empty()) _keep_alive = optKeepAlive;
    if (!_cserverOpts.get_option("--maxpost")->empty()) _max_post_size = data_volume(optMaxPostSize);
    if (!_cserverOpts.get_option("--initscript")->empty()) _initscript = optInitscript;

    if (!_cserverOpts.get_option("--logfile")->empty()) _logfile = optLogfile;
    if (!_cserverOpts.get_option("--loglevel")->empty()) _loglevel = optLogLevel;
    if (!cserverOpts.get_option("--routes")->empty()) {
        std::vector<std::string> rinfos = cserve::split(optRoutes, ';');
        for (const std::string& rinfostr: rinfos) {
            std::vector<std::string> rinfo = cserve::split(rinfostr, ':');
            if (rinfo.size() < 3) {
                std::cerr << fmt::format("Route spcification invalid: {}\n", rinfostr);
                continue;
            }
            if (cserve::strtoupper(rinfo[0]) == "GET") {
                _routes.push_back(cserve::LuaRoute{cserve::Connection::HttpMethod::GET, rinfo[1], rinfo[2]});
            } else if (cserve::strtoupper(rinfo[0]) == "PUT") {
                _routes.push_back(cserve::LuaRoute{cserve::Connection::HttpMethod::PUT, rinfo[1], rinfo[2]});
            } else if (cserve::strtoupper(rinfo[0]) == "POST") {
                _routes.push_back(cserve::LuaRoute{cserve::Connection::HttpMethod::POST, rinfo[1], rinfo[2]});
            } else if (cserve::strtoupper(rinfo[0]) == "DELETE") {
                _routes.push_back(cserve::LuaRoute{cserve::Connection::HttpMethod::DELETE, rinfo[1], rinfo[2]});
            } else if (cserve::strtoupper(rinfo[0]) == "OPTIONS") {
                _routes.push_back(cserve::LuaRoute{cserve::Connection::HttpMethod::OPTIONS, rinfo[1], rinfo[2]});
            } else if (cserve::strtoupper(rinfo[0]) == "CONNECT") {
                _routes.push_back(cserve::LuaRoute{cserve::Connection::HttpMethod::CONNECT, rinfo[1], rinfo[2]});
            } else if (cserve::strtoupper(rinfo[0]) == "HEAD") {
                _routes.push_back(cserve::LuaRoute{cserve::Connection::HttpMethod::HEAD, rinfo[1], rinfo[2]});
            } else if (cserve::strtoupper(rinfo[0]) == "OTHER") {
                _routes.push_back(cserve::LuaRoute{cserve::Connection::HttpMethod::OTHER, rinfo[1], rinfo[2]});
            }
        }
    }
    _filehandler_info = {_filehandler_route, _docroot};
}

void CserverConf::parse_cmdline_args(int argc, char *argv[]) {

}