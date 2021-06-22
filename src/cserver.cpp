#include <string>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <csignal>
#include <utility>

#include "Error.h"
#include "Cserve.h"
#include "LuaServer.h"
#include "LuaSqlite.h"

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
    std::string userid;
    int port = 4711;
    int ssl_port = -1;
#ifdef cserve_ENABLE_SSL
    std::string ssl_certificate;
    std::string ssl_key;
    std::string jwt_secret;
#endif
    int nthreads = 4;
    std::string configfile;
    std::string docroot;
    std::string tmpdir;
    std::string scriptdir;
    std::vector <cserve::LuaRoute> routes;
    std::pair <std::string, std::string> filehandler_info;
    int keep_alive = 20;
    size_t max_post_size = 0;

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
    if (!configfile.empty()) {
        try {
            cserve::LuaServer luacfg = cserve::LuaServer(configfile);
            userid = luacfg.configString("cserve", "userid", "");
            port = luacfg.configInteger("cserve", "port", 4711);
#ifdef cserve_ENABLE_SSL
            ssl_port = luacfg.configInteger("cserve", "ssl_port", -1);
            ssl_certificate = luacfg.configString("cserve", "ssl_certificate", "");
            ssl_key = luacfg.configString("cserve", "ssl_key", "");
            jwt_secret = luacfg.configString("cserve", "jwt_secret", "0123456789ABCDEF0123456789ABCDEF");
#endif
            docroot = luacfg.configString("cserve", "docroot", ".");
            tmpdir = luacfg.configString("cserve", "tmpdir", "/tmp");
            scriptdir = luacfg.configString("cserve", "scriptdir", "./scripts");
            nthreads = luacfg.configInteger("cserve", "nthreads", 4);
            keep_alive = luacfg.configInteger("cserve", "keep_alive", 20);
            max_post_size = luacfg.configInteger("cserve", "max_post_size", 0);
            std::string s;
            std::string initscript = luacfg.configString("cserve", "initscript", s);
            routes = luacfg.configRoute("routes");
        } catch (cserve::Error &err) {
            std::cerr << err << std::endl;
        }
    }

    cserve::Server server(port, nthreads, userid); // instantiate the server
#ifdef cserve_ENABLE_SSL
    server.ssl_port(ssl_port); // set the secure connection port (-1 means no ssl socket)
    if (!ssl_certificate.empty()) server.ssl_certificate(ssl_certificate);
    if (!ssl_key.empty()) server.ssl_key(ssl_key);
    server.jwt_secret(jwt_secret);
#endif
    server.tmpdir(tmpdir); // set the directory for storing temporary files during upload
    server.scriptdir(scriptdir); // set the directory where the Lua scripts are found for the "Lua"-routes
    server.max_post_size(max_post_size); // set the maximal post size
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
