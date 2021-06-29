//
// Created by Lukas Rosenthaler on 28.06.21.
//

#include <thread>

#include "CLI11.hpp"

#include "CserverConf.h"

extern size_t data_volume(const std::string volstr) {
    size_t l = volstr.length();
    size_t ll = 0;
    size_t data_volume = 0;
    char c = '\0';

    if (l > 1) {
        c = toupper(volstr[l - 1]);
        ll = 1;
    }
    if ((l > 2) && (c == 'B')) {
        c = toupper(volstr[l - 2]);
        ll = 2;
    }
    if (c == 'M') {
        data_volume = stoll(volstr.substr(0, l - ll)) * 1024 * 1024;
    } else if (c == 'G') {
        data_volume = stoll(volstr.substr(0, l - ll)) * 1024 * 1024 * 1024;
    } else if (c == 'T') {
        data_volume = stoll(volstr.substr(0, l - ll)) * 1024 * 1024 * 1024 * 1024;
    } else {
        data_volume = stoll(volstr);
    }
    return data_volume;
}

CserverConf::CserverConf(int argc, char *argv[]) {
    //
    // first we set the hardcoded defaults that are used if nothing else has been defined
    //
    _serverconf_ok = 0;

    _userid = "";
    _port = 80;
#ifdef CSERVE_ENABLE_SSL
    _ssl_port = 443;
    std::string ssl_certificate = "./certificate/certificate.pem";
    std::string ssl_key = "./certificate/key.pem";
    std::string jwt_secret = "UP4014, the biggest steam engine";
#else
    _ssl_port = -1;
#endif
    _nthreads = std::thread::hardware_concurrency();
    _docroot = "./docroot";
    _tmpdir = "./tmp";
    _scriptdir = "./scripts";
    _routes = {};
    _filehandler_info = {"/", _docroot};
    _keep_alive = 5;
    _max_post_size = 20*1024*1024; // 20MB
    _initscript = "./scripts/cserver.init.lua";
    _logfile = "./cserver.log";
    _loglevel = spdlog::level::debug;

    //
    // now we parse the command line and environment variables using CLI11
    //
    CLI::App cserverOpts("cserver is a small C++ based webserver with Lua integration.");
    std::string optConfigfile;
    cserverOpts.add_option("-c,--config",
                           optConfigfile,
                           "Configuration file for web server.")->envname("CSERVER_CONFIGFILE")
            ->check(CLI::ExistingFile);

    std::string optUserid;
    cserverOpts.add_option("--userid", optUserid, "Userid to use to run cserver.")
            ->envname("CSERVER_USERID");

    int optServerport = 80;
    cserverOpts.add_option("--port", optServerport, "Standard HTTP port of cserver.")
            ->envname("CSERVER_PORT");

#ifdef cserve_ENABLE_SSL
    int optSSLport;
    cserverOpts.add_option("--sslport", optSSLport, "SSL-port of cserver.")
    ->envname("CSERVER_SSLPORT");

    std::string optSSLCertificatePath;
    cserverOpts.add_option("--sslcert", optSSLCertificatePath, "Path to SSL certificate.")
    ->envname("CSERVER_SSLCERTIFICATE");

    std::string optSSLKeyPath;
    cserverOpts.add_option("--sslkey", optSSLKeyPath, "Path to the SSL key file.")
    ->envname("CSERVER_SSLKEY");

    std::string optJWTKey;
    cserverOpts.add_option("--jwtkey",
                       optJWTKey,
                       "The secret for generating JWT's (JSON Web Tokens) (exactly 42 characters).")
                       ->envname("CSERVER_JWTKEY");
#endif

    std::string optDocroot;
    cserverOpts.add_option("--docroot",
                           optDocroot,
                           "Path to document root for normal webserver.")
            ->envname("CSERVER_DOCROOT")
            ->check(CLI::ExistingDirectory);

    std::string optTmpdir;
    cserverOpts.add_option("--tmpdir",
                           optTmpdir, "Path to the temporary directory (e.g. for uploads etc.).")
            ->envname("CSERVER_TMPDIR")->check(CLI::ExistingDirectory);

    std::string optScriptDir;
    cserverOpts.add_option("--scriptdir",
                           optScriptDir,
                           "Path to directory containing Lua scripts to implement routes.")
            ->envname("CSERVER_SCRIPTDIR")->check(CLI::ExistingDirectory);

    std::string optRoutes;
    cserverOpts.add_option("--routes",
                           optRoutes,
                           "Routing table \"OP:ROUTE:SCRIPT;...\".")
                           ->envname("CSERVER_ROUTES");

    int optNThreads;
    cserverOpts.add_option("-t,--nthreads", optNThreads, "Number of threads for cserver")
            ->envname("CSERVER_NTHREADS");

    int optKeepAlive;
    cserverOpts.add_option("--keepalive",
                           optKeepAlive,
                           "Number of seconds for the keep-alive option of HTTP 1.1.")
            ->envname("CSERVER_KEEPALIVE");

    std::string optMaxPostSize;
    cserverOpts.add_option("--maxpost",
                           optMaxPostSize,
                           "A string indicating the maximal size of a POST request, e.g. '100M'.")
            ->envname("CSERVER_MAXPOSTSIZE");

    std::string optInitscript;
    cserverOpts.add_option("--initscript",
                           optInitscript,
                           "Path to LUA init script.")
            ->envname("CSERVER_INITSCRIPT")
            ->check(CLI::ExistingFile);

    std::string optLogfile;
    cserverOpts.add_option("--logfile", optLogfile, "Name of the logfile.")->envname("CSERVE_LOGFILE");

    spdlog::level::level_enum optLogLevel;
    std::vector<std::pair<std::string, spdlog::level::level_enum>> logLevelMap {
            {"TRACE", spdlog::level::trace},
            {"DEBUG", spdlog::level::debug},
            {"INFO", spdlog::level::info},
            {"WARN", spdlog::level::warn},
            {"ERR", spdlog::level::err},
            {"CRITICAL", spdlog::level::critical},
            {"OFF", spdlog::level::off}
    };
    cserverOpts.add_option("--loglevel",
                           optLogLevel,
                           "Logging level Value can be: 'TRACE', 'DEBUG', 'INFO', 'WARN', 'ERR', 'CRITICAL', 'OFF'.")
            ->transform(CLI::CheckedTransformer(logLevelMap, CLI::ignore_case))
            ->envname("CSERVER_LOGLEVEL");


    try {
        cserverOpts.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        _serverconf_ok = cserverOpts.exit(e);
        return;
    }

    if (!optConfigfile.empty()) {
        try {
            // cserve::Server::logger()->info("Reading configuration from '{}'.", optConfigfile);
            cserve::LuaServer luacfg = cserve::LuaServer(optConfigfile);

            _userid = luacfg.configString("cserve", "userid", _userid);
            _port = luacfg.configInteger("cserve", "port", _port);

#ifdef cserve_ENABLE_SSL
            _ssl_port = luacfg.configInteger("cserve", "ssl_port", _ssl_port);
            _ssl_certificate = luacfg.configString("cserve", "sslcert", _ssl_certificate);
            _ssl_key = luacfg.configString("cserve", "sslkey", _ssl_key);
            _jwt_secret = luacfg.configString("cserve", "jwt_secret", _jwt_secret);
#endif
            _docroot = luacfg.configString("cserve", "docroot", _docroot);
            _tmpdir = luacfg.configString("cserve", "tmpdir", _tmpdir);
            _scriptdir = luacfg.configString("cserve", "scriptdir", _scriptdir);
            _nthreads = luacfg.configInteger("cserve", "nthreads", _nthreads);
            _keep_alive = luacfg.configInteger("cserve", "keep_alive", _keep_alive);
            _max_post_size = luacfg.configInteger("cserve", "max_post_size", _max_post_size);
            _initscript = luacfg.configString("cserve", "initscript", _initscript);
            _logfile = luacfg.configString("cserve", "logfile", _logfile);

            std::string loglevelstr;
            for (const auto ll: logLevelMap) { // convert spdlog::level::level_enum to std::string
                if (ll.second == _loglevel) {
                    loglevelstr = ll.first;
                    break;
                }
            }
            loglevelstr = luacfg.configString("cserve", "loglevel", loglevelstr);
            for (const auto ll: logLevelMap) { // convert std::string to spdlog::level::level_enum
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
    // now merge config file, commandline and environment varliabler parameters
    // config file is overwritten by environment variable is overwritten by command line param
    //
    if (!cserverOpts.get_option("--userid")->empty()) _userid = optUserid;
    if (!cserverOpts.get_option("--port")->empty()) _port = optServerport;
#ifdef cserve_ENABLE_SSL
    if (!cserverOpts.get_option("--sslport")->empty()) ssl_port = optSSLport;
    if (!cserverOpts.get_option("--sslcert")->empty()) ssl_certificate = optSSLCertificatePath;
    if (!cserverOpts.get_option("--sslkey")->empty()) ssl_key = optSSLKeyPath;
    if (!cserverOpts.get_option("--jwtkey")->empty()) jwt_secret = optJWTKey;
#endif
    if (!cserverOpts.get_option("--docroot")->empty()) _docroot = optDocroot;
    if (!cserverOpts.get_option("--tmpdir")->empty()) _tmpdir = optTmpdir;
    if (!cserverOpts.get_option("--scriptdir")->empty()) _scriptdir = optScriptDir;
    if (!cserverOpts.get_option("--nthreads")->empty()) _nthreads = optNThreads;
    if (!cserverOpts.get_option("--keepalive")->empty()) _keep_alive = optKeepAlive;
    if (!cserverOpts.get_option("--maxpost")->empty()) _max_post_size = data_volume(optMaxPostSize);
    if (!cserverOpts.get_option("--initscript")->empty()) _initscript = optInitscript;

    if (!cserverOpts.get_option("--logfile")->empty()) _logfile = optLogfile;
    if (!cserverOpts.get_option("--loglevel")->empty()) _loglevel = optLogLevel;


}