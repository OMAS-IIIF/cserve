#include <string>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <csignal>
#include <utility>
#include <thread>

#include "Error.h"
#include "Cserve.h"
#include "LuaServer.h"
#include "LuaSqlite.h"
#include "CLI11.hpp"

#include "CserverVersion.h"

cserve::Server *serverptr = nullptr;

static sig_t old_sighandler;
static sig_t old_broken_pipe_handler;

static void sighandler(int sig) {
    if (serverptr != nullptr) {
        int old_ll = setlogmask(LOG_MASK(LOG_INFO));
        syslog(LOG_INFO, "Got SIGINT, stopping server");
        setlogmask(old_ll);
        serverptr->stop();
    } else {
        exit(0);
    }
}


/*LUA TEST****************************************************************************/
static int lua_gaga(lua_State *L) {
    lua_getglobal(L, cserve::luaconnection); // push onto stack
    cserve::Connection *conn = (cserve::Connection *) lua_touserdata(L, -1); // does not change the stack
    lua_remove(L, -1); // remove from stack

    int top = lua_gettop(L);

    for (int i = 1; i <= top; i++) {
        const char *str = lua_tostring(L, i);
        if (str != nullptr) {
            conn->send("GAGA: ", 5);
            conn->send(str, strlen(str));
        }
    }
    return 0;
}
//=========================================================================


/*!
 * Just some testing testing the extension of lua, not really doing something useful
 */
static void new_lua_func(lua_State *L, cserve::Connection &conn, void *user_data) {
    lua_pushcfunction(L, lua_gaga);
    lua_setglobal(L, "gaga");
}
//=========================================================================

/*LUA TEST****************************************************************************/


void RootHandler(cserve::Connection &conn, cserve::LuaServer &luaserver, void *user_data, void *dummy) {
    conn.setBuffer();
    std::vector <std::string> headers = conn.header();
    for (unsigned i = 0; i < headers.size(); i++) {
        conn << headers[i] << " : " << conn.header(headers[i]) << "\n";
    }
    conn << "URI: " << conn.uri() << "\n";
    conn << "It works!" << cserve::Connection::flush_data;
    return;
}


void TestHandler(cserve::Connection &conn, cserve::LuaServer &luaserver, void *user_data, void *dummy) {
    conn.setBuffer();
    conn.setChunkedTransfer();

    std::vector <std::string> headers = conn.header();
    for (unsigned i = 0; i < headers.size(); i++) {
        std::cerr << headers[i] << " : " << conn.header(headers[i]) << std::endl;
    }

    if (!conn.getParams("gaga").empty()) {
        std::cerr << "====> gaga = " << conn.getParams("gaga") << std::endl;
    }

    conn.header("Content-Type", "text/html; charset=utf-8");
    conn << "<html><head>";
    conn << "<title>SIPI TEST (chunked transfer)</title>";
    conn << "</head>" << cserve::Connection::flush_data;

    conn << "<body><h1>SIPI TEST (chunked transfer)</h1>";
    conn << "<p>Dies ist ein kleiner Text</p>";
    conn << "</body></html>" << cserve::Connection::flush_data;
    return;
}


int main(int argc, char *argv[]) {
    std::string userid = "";
    int port = 4711;
    int ssl_port = -1;
#ifdef cserve_ENABLE_SSL
    std::string ssl_certificate;
    std::string ssl_key;
    std::string jwt_secret;
#endif
    int nthreads = std::thread::hardware_concurrency();
    std::string configfile;
    std::string docroot = ".";
    std::string tmpdir = "/tmp";
    std::string scriptdir = "./scripts";
    std::vector <cserve::LuaRoute> routes;
    std::pair <std::string, std::string> filehandler_info;
    int keep_alive = 5;
    size_t max_post_size = 20*1024*1024; // 20MB
    std::string initscript = "./cserve.init.lua";

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
    int optSSLport = 443;
    cserverOpts.add_option("--sslport", optSSLport, "SSL-port of cserver.")
    ->envname("CSERVER_SSLPORT");

    std::string optSSLCertificatePath = "./certificate/certificate.pem";
    cserverOpts.add_option("--sslcert", optSSLCertificatePath, "Path to SSL certificate.")
    ->envname("CSERVER_SSLCERTIFICATE");

    std::string optSSLKeyPath = "./certificate/key.pem";
    cserverOpts.add_option("--sslkey", optSSLKeyPath, "Path to the SSL key file.")
    ->envname("CSERVER_SSLKEY");

    std::string optJWTKey = "UP4014, the biggest steam engine";
    cserverOpts.add_option("--jwtkey",
                       optJWTKey,
                       "The secret for generating JWT's (JSON Web Tokens) (exactly 42 characters).")
                       ->envname("CSERVER_JWTKEY");
#endif

    std::string optDocroot = "./server";
    cserverOpts.add_option("--docroot",
                           optDocroot,
                           "Path to document root for normal webserver.")
            ->envname("CSERVER_DOCROOT")
            ->check(CLI::ExistingDirectory);

    std::string optTmpdir = "./tmp";
    cserverOpts.add_option("--tmpdir",
                           optTmpdir, "Path to the temporary directory (e.g. for uploads etc.).")
            ->envname("CSERVER_TMPDIR")->check(CLI::ExistingDirectory);

    std::string optScriptDir = "./scripts";
    cserverOpts.add_option("--scriptdir",
                           optScriptDir,
                           "Path to directory containing Lua scripts to implement routes.")
            ->envname("CSERVER_SCRIPTDIR")->check(CLI::ExistingDirectory);

    int optNThreads = std::thread::hardware_concurrency();
    cserverOpts.add_option("-t,--nthreads", optNThreads, "Number of threads for cserver")
    ->envname("CSERVER_NTHREADS");

    int optKeepAlive = 5;
    cserverOpts.add_option("--keepalive",
                           optKeepAlive,
                           "Number of seconds for the keep-alive option of HTTP 1.1.")
                           ->envname("CSERVER_KEEPALIVE");

    std::string optMaxPostSize = "100M";
    cserverOpts.add_option("--maxpost",
                       optMaxPostSize,
                       "A string indicating the maximal size of a POST request, e.g. '100M'.")
                       ->envname("CSERVER_MAXPOSTSIZE");


    enum class LogLevel { DEBUG, INFO, NOTICE, WARNING, ERR, CRIT, ALERT, EMERG };
    LogLevel optLogLevel = LogLevel::DEBUG;
    std::vector<std::pair<std::string, LogLevel>> logLevelMap{
            {"DEBUG", LogLevel::DEBUG},
            {"INFO", LogLevel::INFO},
            {"NOTICE", LogLevel::NOTICE},
            {"WARNING", LogLevel::WARNING},
            {"ERR", LogLevel::ERR},
            {"CRIT", LogLevel::CRIT},
            {"ALERT", LogLevel::ALERT},
            {"EMERG", LogLevel::EMERG}
    };
    cserverOpts.add_option("--loglevel",
                           optLogLevel,
                           "Logging level Value can be: 'DEBUG', 'INFO', 'WARNING', 'ERR', 'CRIT', 'ALERT', 'EMERG'.")
                           ->transform(CLI::CheckedTransformer(logLevelMap, CLI::ignore_case))
                           ->envname("CSERVER_LOGLEVEL");

/*
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-p") == 0) || (strcmp(argv[i], "-port") == 0)) {
            i++;
            if (i < argc) port = atoi(argv[i]);
        } else if ((strcmp(argv[i], "-c") == 0) || (strcmp(argv[i], "-config") == 0)) {
            i++;
            if (i < argc) configfile = argv[i];
        } else if ((strcmp(argv[i], "-d") == 0) || (strcmp(argv[i], "-docroot") == 0)) {
            i++;
            if (i < argc) docroot = argv[i];
        } else if ((strcmp(argv[i], "-t") == 0) || (strcmp(argv[i], "-tmpdir") == 0)) {
            i++;
            if (i < argc) tmpdir = argv[i];
        } else if ((strcmp(argv[i], "-n") == 0) || (strcmp(argv[i], "-nthreads") == 0)) {
            i++;
            if (i < argc) nthreads = atoi(argv[i]);
        } else if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "-help") == 0) ||
                   (strcmp(argv[i], "--help") == 0)) {
            std::cerr << "usage:" << std::endl;
            std::cerr
                    << "shttp-test [-p|-port <int def=4711>] [-c|-config <filename>] [-d|-docroot <path>] [-t|-tmpdir <path>] [-n|-nthreads <int def=4>]"
                    << std::endl << std::endl;
            return 0;
        }
    }
*/
    /*
     * Form if config file:
     *
     * cserve = {
     *    docroot = '/Volumes/data/cserve-docroot',
     *    tmpdir = '/tmp',
     *    port = 8080,
     *    nthreads = 16
     * }
     */
    if (!optConfigfile.empty()) {
        try {
            cserve::LuaServer luacfg = cserve::LuaServer(optConfigfile);

            userid = luacfg.configString("cserve", "userid", userid);
            port = luacfg.configInteger("cserve", "port", port);

#ifdef cserve_ENABLE_SSL
            ssl_port = luacfg.configInteger("cserve", "ssl_port", -1);
            ssl_certificate = luacfg.configString("cserve", "sslcert", "");
            ssl_key = luacfg.configString("cserve", "sslkey", "");
            jwt_secret = luacfg.configString("cserve", "jwt_secret", "0123456789ABCDEF0123456789ABCDEF");
#endif
            docroot = luacfg.configString("cserve", "docroot", docroot);
            tmpdir = luacfg.configString("cserve", "tmpdir", tmpdir);
            scriptdir = luacfg.configString("cserve", "scriptdir", scriptdir);
            nthreads = luacfg.configInteger("cserve", "nthreads", nthreads);
            keep_alive = luacfg.configInteger("cserve", "keep_alive", keep_alive);

            max_post_size = luacfg.configInteger("cserve", "max_post_size", max_post_size);

            initscript = luacfg.configString("cserve", "initscript", initscript);
            routes = luacfg.configRoute("routes");
        } catch (cserve::Error &err) {
            std::cerr << err << std::endl;
        }
    }
    if (!cserverOpts.get_option("--userid")->empty()) userid = optUserid;
    if (!cserverOpts.get_option("--port")->empty()) port = optServerport;
#ifdef cserve_ENABLE_SSL
    if (!cserverOpts.get_option("--sslport")->empty()) ssl_port = optSSLport;
    if (!cserverOpts.get_option("--sslcert")->empty()) ssl_certificate = optSSLCertificatePath;
    if (!cserverOpts.get_option("--sslkey")->empty()) ssl_key = optSSLKeyPath;
    if (!cserverOpts.get_option("--jwtkey")->empty()) jwt_secret = optJWTKey;
#endif
    if (!cserverOpts.get_option("--docroot")->empty()) docroot = optDocroot;
    if (!cserverOpts.get_option("--tmpdir")->empty()) tmpdir = optTmpdir;
    if (!cserverOpts.get_option("--scriptdir")->empty()) scriptdir = optScriptDir;
    if (!cserverOpts.get_option("--nthreads")->empty()) nthreads = optNThreads;
    if (!cserverOpts.get_option("--keepalive")->empty()) nthreads = optKeepAlive;
    if (!cserverOpts.get_option("--maxpost")->empty()) {
        size_t l = optMaxPostSize.length();
        char c = optMaxPostSize[l - 1];
        if (c == 'M') {
            max_post_size = stoll(optMaxPostSize.substr(0, l - 1)) * 1024 * 1024;
        } else if (c == 'G') {
            max_post_size = stoll(optMaxPostSize.substr(0, l - 1)) * 1024 * 1024 * 1024;
        } else {
            max_post_size = stoll(optMaxPostSize);
        }
    }


    cserve::Server server(port, nthreads, userid); // instantiate the server
#ifdef cserve_ENABLE_SSL
    server.ssl_port(ssl_port); // set the secure connection port (-1 means no ssl socket)
    if (!ssl_certificate.empty()) server.ssl_certificate(ssl_certificate);
    if (!ssl_key.empty()) server.ssl_key(ssl_key);
    server.jwt_secret(jwt_secret);
#endif
    std::cerr << "3--------------->" << tmpdir << "<---------" << std::endl;
    server.tmpdir(tmpdir); // set the directory for storing temporary files during upload
    server.scriptdir(scriptdir); // set the directory where the Lua scripts are found for the "Lua"-routes
    server.max_post_size(max_post_size); // set the maximal post size
    //server.loglevel();
    server.luaRoutes(routes);
    server.keep_alive_timeout(keep_alive); // set the keep alive timeout
    server.add_lua_globals_func(cserve::sqliteGlobals, &server);
    server.add_lua_globals_func(new_lua_func); // add new lua function "gaga"

    //
    // now we set the routes for the normal HTTP server file handling
    //
    if (!docroot.empty()) {
        filehandler_info.first = "/";
        filehandler_info.second = docroot;
        server.addRoute(cserve::Connection::GET, "/", cserve::FileHandler, &filehandler_info);
        server.addRoute(cserve::Connection::POST, "/", cserve::FileHandler, &filehandler_info);
    }

    //
    // Test handler (should be removed for production system)
    //
    server.addRoute(cserve::Connection::GET, "/test", TestHandler, nullptr);

    serverptr = &server;
    old_sighandler = signal(SIGINT, sighandler);
    old_broken_pipe_handler = signal(SIGPIPE, SIG_IGN);

    server.run();
    std::cerr << "SERVER HAS FINISHED ITS SERVICE" << std::endl;
    (void) signal(SIGINT, old_sighandler);
    (void) signal(SIGPIPE, old_broken_pipe_handler);
}
