#include <string>
#include <iostream>
#include <cstdlib>
#include <csignal>
#include <utility>
#include <filesystem>
#include <csignal>

#include "Cserve.h"
#include "LuaServer.h"
#include "LuaSqlite.h"
#include "CserverConf.h"
#include "RequestHandler.h"

#include "RequestHandlerLoader.h"

#include "../handlers/testhandler/TestHandler.h"
#include "../handlers/pinghandler/PingHandler.h"
#include "../handlers/scripthandler/ScriptHandler.h"
#include "../handlers/filehandler/FileHandler.h"
#include "../handlers/iiifhandler/IIIFHandler.h"


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

void my_terminate_handler() {
    try {
        // Rethrow the current exception to identify it
        throw;
    } catch (const std::exception& e) {
        syslog(LOG_ERR, "Unhandled exception caught: %s", e.what());
    } catch (...) {
        syslog(LOG_ERR, "Unhandled unknown exception caught");
    }
    std::abort(); // Abort the program or perform other cleanup
}

int main(int argc, char *argv[]) {
    (void) std::signal(SIGINT, old_sighandler);
    (void) std::signal(SIGPIPE, old_broken_pipe_handler);
    std::set_terminate(my_terminate_handler);

    auto logger = cserve::Server::create_logger(spdlog::level::trace);
    std::unordered_map<std::string, std::shared_ptr<cserve::RequestHandler>> handlers;

    int old_ll = setlogmask(LOG_MASK(LOG_INFO));
    logger->info(cserve::Server::version_string());
    setlogmask(old_ll);

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        logger->info("Current working dir: {}", cwd);
    } else {
        logger->error("getcwd() error");
        return 1;
    }

    //std::string handlerdir("./handler");

    /*
    if (const char* env_p = std::getenv("CSERVE_HANDLERDIR")) {
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
    */
    {
        auto testhandler = std::make_shared<cserve::TestHandler>();
        handlers[testhandler->name()] = testhandler;

        auto pinghandler = std::make_shared<cserve::PingHandler>();
        handlers[pinghandler->name()] = pinghandler;

        auto filehandler = std::make_shared<cserve::FileHandler>();
        handlers[filehandler->name()] = filehandler;

        auto scripthandler = std::make_shared<cserve::ScriptHandler>();
        handlers[scripthandler->name()] = scripthandler;

        auto iiifhandler = std::make_shared<cserve::IIIFHandler>();
        handlers[iiifhandler->name()] = iiifhandler;
    }

    //
    // read the configuration parameters. The parameters can be defined in
    // 1) a lua configuration parameter file
    // 2) environment variables
    // 3) commandline parameters.
    // The configuration file parameters (lowest priority) are superseeded by the environment variables are
    // superseeded by the command line parameters (highest priority)
    //
    cserve::CserverConf config;
    const std::string prefix{"cserve"};

    //config.add_config(prefix, "handlerdir", "./handler", "Path to dirctory containing the handler plugins.");
    config.add_config(prefix, "config", "./config", "Configuration file for web server.");
    config.add_config(prefix, "userid", "", "Username to use to run cserver. Mus be launched as root to use this option");
    config.add_config(prefix, "port", 8080, "HTTP port to be used [default=8080]");
    config.add_config(prefix, "sslport", 8443, "SHTTP port to be used (SLL) [default=8443]");
    config.add_config(prefix, "sslcert", "./certificate/certificate.pem", "Path to SSL certificate.");
    config.add_config(prefix, "sslkey", "./certificate/key.pem", "Path to the SSL key file.");
    config.add_config(prefix, "jwtkey", "UP4014, the biggest steam engine", "The secret for generating JWT's (JSON Web Tokens) (exactly 42 characters).");
    config.add_config(prefix, "nthreads", static_cast<int>(std::thread::hardware_concurrency()), "Number of worker threads to be used by cserver");
    config.add_config(prefix, "tmpdir", "./tmp", "Path to the temporary directory (e.g. for uploads etc.).");
    config.add_config(prefix, "keepalive", 10, "Number of seconds for the keep-alive option of HTTP 1.1. Set to 0 for no keep_alive. [default=10]");
    config.add_config(prefix, "maxpost", cserve::DataSize("1MB"), "A string indicating the maximal size of a POST request, e.g. '100M'.");
    config.add_config(prefix, "lua_include_path", "./scripts", "Include path for Lua.");
    config.add_config(prefix, "initscript", "", "Path to LUA init script.");
    config.add_config(prefix, "logfile", "./cserver.log", "Name of the logfile.");
    config.add_config(prefix, "loglevel", spdlog::level::debug, "Logging level Value can be: 'TRACE', 'DEBUG', 'INFO', 'WARN', 'ERR', 'CRITICAL', 'OFF'.");

    //
    // load the configuration variables of the plugin handlers
    //
    for (auto & [name, handler]: handlers) {
        handler->set_config_variables(config);
    }

    config.parse_cmdline_args(argc, (const char **) argv);

    //if (config.serverconf_ok() != 0) return config.serverconf_ok();

    logger->set_level(config.get_loglevel("loglevel").value());

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
    server.lua_include_path(config.get_string("lua_include_path").value());
    std::string initscript = config.get_string("initscript").value();
    if (!initscript.empty()) server.initscript(initscript);
    server.max_post_size(config.get_datasize("maxpost").value().as_size_t()); // set the maximal post size
    server.keep_alive_timeout(config.get_int("keepalive").value()); // set the keep alive timeout

    //
    // initialize Lua with some "extensions" and global variables
    //
    server.add_lua_globals_func(cserve::cserverConfGlobals, &config);
    server.add_lua_globals_func(cserve::sqliteGlobals, &server);
    server.add_lua_globals_func(new_lua_func); // add new lua function "gaga"

    //
    // setup the routes for the plugin handlers
    //
    for (auto & [name, handler]: handlers) {
        handler->get_config_variables(config);
        //std::string routeopt = handler->name() + "_routes";
        auto routes = config.get_luaroutes(handler->name(), "routes").value();
        for (auto & route: routes) {
            server.addRoute(route.method, route.route, handler);
            handler->add_route_data(route.route, route.additional_data);
            old_ll = setlogmask(LOG_MASK(LOG_INFO));
            logger->info("Added route: handler: '{}' method: '{}', route: '{}' route data: '{}'", name, route.method_as_string(), route.route, route.additional_data);
            setlogmask(old_ll);
        }
    }

    serverptr = &server;
    old_sighandler = std::signal(SIGINT, sighandler);
    old_broken_pipe_handler = signal(SIGPIPE, SIG_IGN);
    server.run();
    logger->info("CSERVE has finished it's service");
}
