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
    logger->info("CSERVER started main");

    //
    // read the configuration parameters. The parameters can be defined in
    // 1) a lua configuration parameter file
    // 2) environment variables
    // 3) commandline parameters.
    // The configuration file parameters (lowest priority) are superseeded by the environment variables are
    // superseeded by the command line parameters (highest priority)
    //
    CserverConf config(argc, argv);
    if (config.serverconf_ok() != 0) return config.serverconf_ok();

    logger->set_level(config.loglevel());

    cserve::Server server(config.port(), config.nthreads(), config.userid()); // instantiate the server

    std::cout << cserve::Server::version_string() << std::endl;

    server.ssl_port(config.ssl_port()); // set the secure connection port (-1 means no ssl socket)
    if (!config.ssl_certificate().empty()) server.ssl_certificate(config.ssl_certificate());
    if (!config.ssl_key().empty()) server.ssl_key(config.ssl_key());
    server.jwt_secret(config.jwt_secret());
    server.tmpdir(config.tmpdir()); // set the directory for storing temporary files during upload
    server.scriptdir(config.scriptdir()); // set the directory where the Lua scripts are found for the "Lua"-routes
    if (!config.initscript().empty()) server.initscript(config.initscript());
    server.max_post_size(config.max_post_size()); // set the maximal post size
    server.luaRoutes(config.routes());
    server.keep_alive_timeout(config.keep_alive()); // set the keep alive timeout

    //
    // initialize Lua with some "extensions" and global variables
    //
    server.add_lua_globals_func(cserverConfGlobals, &config);
    server.add_lua_globals_func(cserve::sqliteGlobals, &server);
    server.add_lua_globals_func(new_lua_func); // add new lua function "gaga"

    //
    // now we set the routes for the normal HTTP server file handling
    //
    if (!config.docroot().empty()) {
        std::pair <std::string, std::string> tmp = config.filehandler_info();
        auto file_handler = std::make_shared<cserve::FileHandler>(tmp.first, tmp.second);
        server.addRoute(cserve::Connection::GET, tmp.first, file_handler);
        server.addRoute(cserve::Connection::POST, tmp.first, file_handler);
    }

    //
    // Test handler (should be removed for production system)
    //
    std::filesystem::path testhandler_path(config.handlerdir());
    testhandler_path /= "testhandler.so";
    cserve::RequestHandlerLoader test_loader(testhandler_path,
                                             "createTestHandler",
                                             "destroyTestHandler");
    std::shared_ptr<cserve::RequestHandler> test_handler = test_loader.get_instance();
    server.addRoute(cserve::Connection::GET, "/test", test_handler);

    std::filesystem::path pinghandler_path(config.handlerdir());
    pinghandler_path /= "pinghandler.so";
    cserve::RequestHandlerLoader ping_loader(pinghandler_path,
                                             "createPingHandler",
                                             "destroyPingHandler");
    std::shared_ptr<cserve::RequestHandler> ping_handler = ping_loader.get_instance();
    server.addRoute(cserve::Connection::GET, "/ping", ping_handler);

    serverptr = &server;
    old_sighandler = signal(SIGINT, sighandler);
    old_broken_pipe_handler = signal(SIGPIPE, SIG_IGN);
    server.run();
    logger->info("CSERVE has finished it's service");
    (void) signal(SIGINT, old_sighandler);
    (void) signal(SIGPIPE, old_broken_pipe_handler);
}
