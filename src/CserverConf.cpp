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
    _serverconf_ok = 0;

    _values.emplace("handlerdir", cserve::ConfValue("--handlerdir", "./handler", "", "CSERVER_HANLDERDIR"));
    _cserverOpts->add_option("-c,--config",
                             optConfigfile,
                             "Configuration file for web server.")->envname("CSERVER_CONFIGFILE")
            ->check(CLI::ExistingFile);

    _values.emplace("userid", ConfValue("", "Userid to use to run cserver.", "CSERVER_USERID"));
    _values.emplace("port", ConfValue(8080, "", "CSERVER_PORT"));
    _values.emplace("ssl_port", ConfValue(8443, "HTTP port to use for cserver.", "CSERVER_SSLPORT"));
    _values.emplace("ssl_certificate", ConfValue("./certificate/certificate.pem", "Path to SSL certificate.", "CSERVER_SSLCERTIFICATE"));
    _values.emplace("ssl_key", ConfValue("./certificate/key.pem", "Path to the SSL key file.", "CSERVER_SSLKEY"));
    _values.emplace("jwt_secret", ConfValue("UP4014, the biggest steam engine", "The secret for generating JWT's (JSON Web Tokens) (exactly 42 characters).", "CSERVER_JWTKEY"));
    _values.emplace("nthreads", ConfValue(static_cast<int>(std::thread::hardware_concurrency()), "Number of worker threads to be used by cserver\"", "CSERVER_NTHREADS"));
    _values.emplace("docroot", ConfValue("./docroot", "Path to document root for file server.", "CSERVER_DOCROOT")); // TODO: Move to file server plugin
    _values.emplace("filehandler_route", ConfValue("/", "Route root for file server.", "CSERVER_FILEHANDLER_ROUTE")); // TODO: Move to file server plugin
    // _values.emplace("filehandler_info", ConfValue(std::pair<std::string, std::string>("/", "./docroot"), "File server info (", "CSERVER_FILEHANDLER_INFO")); // TODO: necessary??: no initialization needed. Depends on _docroot and _filehandler_route
    _values.emplace("tmpdir", ConfValue("./tmp", "Path to the temporary directory (e.g. for uploads etc.).", "CSERVER_TMPDIR"));
    _values.emplace("scriptdir", ConfValue("./scripts", "Path to directory containing Lua scripts to implement routes.", "CSERVER_SCRIPTDIR")); // Todo: Move to script handler plugin
    _values.emplace("luaroutes", ConfValue(std::vector<cserve::LuaRoute>(), "Lua routes in the form \"<http-type>:<route>:<script>", "CSERVER_LUAROUTES"));  // Todo: Move to script handler plugin
    _routes = {};
    _values.emplace("keep_alive", ConfValue(5, "Number of seconds for the keep-alive option of HTTP 1.1.", "CSERVER_KEEPALIVE"));
    _values.emplace("max_post_size", ConfValue(DataSize("50MB"), "A string indicating the maximal size of a POST request, e.g. '100M'.", "CSERVER_MAXPOSTSIZE")); // 50MB
    _values.emplace("initscript", ConfValue("", "Path to LUA init script.", "CSERVER_INITSCRIPT"));
    _values.emplace("logfile", ConfValue("./cserver.log", "Name of the logfile.", "CSERVE_LOGFILE"));
    _values.emplace("loglevel", ConfValue(LogLevel(spdlog::level::debug), "Logging level Value can be: 'TRACE', 'DEBUG', 'INFO', 'WARN', 'ERR', 'CRITICAL', 'OFF'.", "CSERVER_LOGLEVEL"));


    for (auto const& [name, val] : _values) {
        auto vtype = val.get_type();
        switch (vtype) {
            case ConfValue::INTEGER:
                break;
            case ConfValue::FLOAT:
                break;
            case ConfValue::STRING:
                break;
            case ConfValue::PAIR:
                break;
            case ConfValue::DATASIZE:
                break;
            case ConfValue::LOGLEVEL:
                break;
        }
        lua_rawset(L, -3); // table1
    }

   ConfValue gaga = _values["port"];

    //
    // now we parse the command line and environment variables using CLI11
    //
    _cserverOpts = std::make_shared<CLI::App>("cserver is a small C++ based webserver with Lua integration.", "cserver");

    std::string optConfigfile;
    _cserverOpts->add_option("-c,--config",
                             optConfigfile,
                             "Configuration file for web server.")->envname("CSERVER_CONFIGFILE")
            ->check(CLI::ExistingFile);

    std::string optHandlerdir;
    _cserverOpts->add_option("--handlerdir", optHandlerdir, "Path to the request handlers.")
            ->envname("CSERVER_HANDLERDIR");

    std::string optUserid;
    _cserverOpts->add_option("--userid", optUserid, "Userid to use to run cserver.")
            ->envname("CSERVER_USERID");

    int optServerport = 8080;
    _cserverOpts->add_option("--port", optServerport, "Standard HTTP port of cserver.")
            ->envname("CSERVER_PORT");

    int optSSLport = 8081;
    _cserverOpts->add_option("--sslport", optSSLport, "SSL-port of cserver.")
            ->envname("CSERVER_SSLPORT");

    std::string optSSLCertificatePath;
    _cserverOpts->add_option("--sslcert", optSSLCertificatePath, "Path to SSL certificate.")
            ->envname("CSERVER_SSLCERTIFICATE");

    std::string optSSLKeyPath;
    _cserverOpts->add_option("--sslkey", optSSLKeyPath, "Path to the SSL key file.")
            ->envname("CSERVER_SSLKEY");

    std::string optJWTKey;
    _cserverOpts->add_option("--jwtkey",
                             optJWTKey,
                             "The secret for generating JWT's (JSON Web Tokens) (exactly 42 characters).")
            ->envname("CSERVER_JWTKEY");

    std::string optDocroot;
    _cserverOpts->add_option("--docroot",
                             optDocroot,
                             "Path to document root for normal webserver.")
            ->envname("CSERVER_DOCROOT")
            ->check(CLI::ExistingDirectory);

    std::string optFilehandlerRoute;
    _cserverOpts->add_option("--filehandler_route",
                             optFilehandlerRoute,
                             "Route root for normal webserver.")
            ->envname("CSERVER_FILEHANDLER_ROUTE");

    std::string optTmpdir;
    _cserverOpts->add_option("--tmpdir",
                             optTmpdir, "Path to the temporary directory (e.g. for uploads etc.).")
            ->envname("CSERVER_TMPDIR")->check(CLI::ExistingDirectory);

    std::string optScriptDir;
    _cserverOpts->add_option("--scriptdir",
                             optScriptDir,
                             "Path to directory containing Lua scripts to implement routes.")
            ->envname("CSERVER_SCRIPTDIR")->check(CLI::ExistingDirectory);

    std::string optRoutes;
    _cserverOpts->add_option("--routes",
                             optRoutes,
                             "Routing table \"OP:ROUTE:SCRIPT;...\".")
            ->envname("CSERVER_ROUTES");

    int optNThreads;
    _cserverOpts->add_option("-t,--nthreads", optNThreads, "Number of threads for cserver")
            ->envname("CSERVER_NTHREADS");

    int optKeepAlive;
    _cserverOpts->add_option("--keepalive",
                             optKeepAlive,
                             "Number of seconds for the keep-alive option of HTTP 1.1.")
            ->envname("CSERVER_KEEPALIVE");

    std::string optMaxPostSize;
    _cserverOpts->add_option("--maxpost",
                             optMaxPostSize,
                             "A string indicating the maximal size of a POST request, e.g. '100M'.")
            ->envname("CSERVER_MAXPOSTSIZE");

    std::string optInitscript;
    _cserverOpts->add_option("--initscript",
                             optInitscript,
                             "Path to LUA init script.")
            ->envname("CSERVER_INITSCRIPT")
            ->check(CLI::ExistingFile);

    std::string optLogfile;
    _cserverOpts->add_option("--logfile", optLogfile, "Name of the logfile.")->envname("CSERVE_LOGFILE");

    spdlog::level::level_enum optLogLevel;
    std::vector<std::pair<std::string, spdlog::level::level_enum>> logLevelMap{
            {"TRACE",    spdlog::level::trace},
            {"DEBUG",    spdlog::level::debug},
            {"INFO",     spdlog::level::info},
            {"WARN",     spdlog::level::warn},
            {"ERR",      spdlog::level::err},
            {"CRITICAL", spdlog::level::critical},
            {"OFF",      spdlog::level::off}
    };
    _cserverOpts->add_option("--loglevel",
                             optLogLevel,
                             "Logging level Value can be: 'TRACE', 'DEBUG', 'INFO', 'WARN', 'ERR', 'CRITICAL', 'OFF'.")
            ->transform(CLI::CheckedTransformer(logLevelMap, CLI::ignore_case))
            ->envname("CSERVER_LOGLEVEL");


    try {
        _cserverOpts->parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        _serverconf_ok = _cserverOpts->exit(e);
        return;
    }

    if (!optConfigfile.empty()) {
        try {
            // cserve::Server::logger()->info("Reading configuration from '{}'.", optConfigfile);
            cserve::LuaServer luacfg = cserve::LuaServer(optConfigfile);

            _handlerdir = luacfg.configString("cserve", "handlerdir", _handlerdir);
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