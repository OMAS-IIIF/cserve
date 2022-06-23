#include <string>
#include <iostream>
#include <cstdlib>
#include <csignal>
#include <utility>
#include <dlfcn.h>


#include "Cserve.h"
#include "LuaServer.h"
#include "LuaSqlite.h"
#include "CserverConf.h"
#include "RequestHandler.h"
#include "PingHandler.h"

#include "RequestHandlerLoader.h"

cserve::Server *serverptr = nullptr;

static sig_t old_sighandler;
static sig_t old_broken_pipe_handler;

static void sighandler(int sig) {
    if (serverptr != nullptr) {
        int old_ll = setlogmask(LOG_MASK(LOG_INFO));
        //syslog(LOG_INFO, "Got SIGINT, stopping server");
        cserve::Server::logger()->info("Got signal {}, stopping server", sig);
        setlogmask(old_ll);
        serverptr->stop();
    } else {
        exit(0);
    }
}


/*LUA TEST****************************************************************************/
static int lua_gaga(lua_State *L) {
    lua_getglobal(L, cserve::luaconnection); // push onto stack
    auto *conn = (cserve::Connection *) lua_touserdata(L, -1); // does not change the stack
    lua_remove(L, -1); // remove from stack

    int top = lua_gettop(L);

    for (int i = 1; i <= top; i++) {
        const char *str = lua_tostring(L, i);
        if (str != nullptr) {
            conn->send("GAGA: ", 5);
            conn->send(str, static_cast<int>(strlen(str)));
        }
    }
    return 0;
}
//=========================================================================


/*!
 * Just some testing testing the extension of lua, not really doing something useful
 */
static void new_lua_func(lua_State *L, cserve::Connection &conn, [[maybe_unused]] void *user_data) {
    lua_pushcfunction(L, lua_gaga);
    lua_setglobal(L, "gaga");
}
//=========================================================================

/*LUA TEST****************************************************************************/

/*
void RootHandler(cserve::Connection &conn, cserve::LuaServer &luaserver, void *user_data, std::shared_ptr<cserve::RequestHandlerData> request_data) {
    conn.setBuffer();
    std::vector <std::string> headers = conn.header();
    for (auto & header : headers) {
        conn << header << " : " << conn.header(header) << "\n";
    }
    conn << "URI: " << conn.uri() << "\n";
    conn << "It works!" << cserve::Connection::flush_data;
    return;
}
*/
int main(int argc, char *argv[]) {
    auto logger = cserve::Server::create_logger();

    int old_ll = setlogmask(LOG_MASK(LOG_INFO));
    logger->info(cserve::Server::version_string());
    setlogmask(old_ll);

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        logger->info("Current working dir: {}", cwd);
    } else {
        logger->error("getcwd() error");
        return 1;
    }

    std::string handlerdir("./handler");

    if (const char* env_p = std::getenv("CSERVER_HANDLERDIR")) {
        handlerdir = env_p;
    }
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--handlerdir") == 0) {
            ++i;
            if (i < argc) {
                handlerdir = argv[i];
                break;
            }
        }
    }

    //
    // Load test handler (should be removed for production system)
    //
    std::filesystem::path testhandler_path(handlerdir);
    testhandler_path /= "testhandler.so";
    cserve::RequestHandlerLoader test_loader(testhandler_path,
                                             "createTestHandler",
                                             "destroyTestHandler");
    std::shared_ptr<cserve::RequestHandler> test_handler = test_loader.get_instance();

    //
    // Load ping handler
    //
    std::filesystem::path pinghandler_path(handlerdir);
    pinghandler_path /= "pinghandler.so";
    cserve::RequestHandlerLoader ping_loader(pinghandler_path,
                                             "createPingHandler",
                                             "destroyPingHandler");
    std::shared_ptr<cserve::RequestHandler> ping_handler = ping_loader.get_instance();

    //
    // read the configuration parameters. The parameters can be defined in
    // 1) a lua configuration parameter file
    // 2) environment variables
    // 3) commandline parameters.
    // The configuration file parameters (lowest priority) are superseeded by the environment variables are
    // superseeded by the command line parameters (highest priority)
    //
    CserverConf config;

    config.add_config("handlerdir", "./handler", "Path to dirctory containing the handler plugins.");
    config.add_config("config", "./config", "Configuration file for web server.");
    config.add_config("userid", "", "Username to use to run cserver. Mus be launched as root to use this option");
    config.add_config("port", 8080, "HTTP port to be used [default=8080]");
    config.add_config("sslport", 8443, "SHTTP port to be used (SLL) [default=8443]");
    config.add_config("sslcert", "./certificate/certificate.pem", "Path to SSL certificate.");
    config.add_config("sslkey", "./certificate/key.pem", "Path to the SSL key file.");
    config.add_config("jwtkey", "UP4014, the biggest steam engine", "The secret for generating JWT's (JSON Web Tokens) (exactly 42 characters).");
    config.add_config("nthreads", static_cast<int>(std::thread::hardware_concurrency()), "Number of worker threads to be used by cserver");
    config.add_config("tmpdir", "./tmp", "Path to the temporary directory (e.g. for uploads etc.).");
    config.add_config("keepalive", 5, "Number of seconds for the keep-alive option of HTTP 1.1.");
    config.add_config("maxpost", cserve::DataSize("50MB"), "A string indicating the maximal size of a POST request, e.g. '100M'.");
    config.add_config("initscript", "", "Path to LUA init script.");
    config.add_config("logfile", "./cserver.log", "Name of the logfile.");
    config.add_config("loglevel", spdlog::level::debug, "Logging level Value can be: 'TRACE', 'DEBUG', 'INFO', 'WARN', 'ERR', 'CRITICAL', 'OFF'.");

    //
    // file server stuff
    //
    config.add_config("docroot", "./docroot", "Path to document root for file server.");
    config.add_config("wwwroute", "/", "Route root for file server.");

    //
    // script handler stuff
    //
    config.add_config("scriptdir", "./scripts", "Path to directory containing Lua scripts to implement routes.");
    //config.add_config("routes", std::vector<cserve::LuaRoute>{}, "Lua routes in the form \"<http-type>:<route>:<script>\"");

    config.parse_cmdline_args(argc, argv);

    std::cerr << "==========> docroot=" << config.get_string("docroot").value() << " <==================" << std::endl;

    //if (config.serverconf_ok() != 0) return config.serverconf_ok();



    logger->set_level(config.loglevel());

    int port = config.get_int("port").value();
    int nthreads = config.get_int("nthreads").value();
    std::string userid = config.get_string("userid").value();

    cserve::Server server(port, static_cast<unsigned>(nthreads), userid); // instantiate the server


    server.ssl_port(config.get_int("sslport").value()); // set the secure connection port (-1 means no ssl socket)
    std::string ssl_certificate = config.get_string("sslcert").value();
    if (!ssl_certificate.empty()) server.ssl_certificate(ssl_certificate);
    std::string ssl_key = config.get_string("sslkey").value();
    if (!ssl_key.empty()) server.ssl_key(ssl_key);
    server.jwt_secret(config.get_string("jwtkey").value());
    server.tmpdir(config.get_string("tmpdir").value());
    server.scriptdir(config.get_string("scriptdir").value());
    std::string initscript = config.get_string("initscript").value();
    if (!initscript.empty()) server.initscript(initscript);
    server.max_post_size(config.get_datasize("maxpost").value().as_size_t()); // set the maximal post size
    server.keep_alive_timeout(config.get_int("keepalive").value()); // set the keep alive timeout
    server.luaRoutes(config.routes());


    //
    // now we set the routes for the normal HTTP server file handling
    //
    std::string wwwroute = config.get_string("wwwroute").value();
    std::string docroot = config.get_string("docroot").value();


    //
    // initialize Lua with some "extensions" and global variables
    //
    server.add_lua_globals_func(cserverConfGlobals, &config);
    server.add_lua_globals_func(cserve::sqliteGlobals, &server);
    server.add_lua_globals_func(new_lua_func); // add new lua function "gaga"



    if (!docroot.empty()) {
        auto file_handler = std::make_shared<cserve::FileHandler>(wwwroute, docroot);
        server.addRoute(cserve::Connection::GET, wwwroute, file_handler);
        server.addRoute(cserve::Connection::POST, wwwroute, file_handler);
    }

    server.addRoute(cserve::Connection::GET, "/test", test_handler);
    server.addRoute(cserve::Connection::GET, "/ping", ping_handler);

    serverptr = &server;
    old_sighandler = signal(SIGINT, sighandler);
    old_broken_pipe_handler = signal(SIGPIPE, SIG_IGN);
    server.run();
    logger->info("CSERVE has finished it's service");
    (void) signal(SIGINT, old_sighandler);
    (void) signal(SIGPIPE, old_broken_pipe_handler);
}
