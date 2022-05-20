//
// Created by Lukas Rosenthaler on 28.06.21.
//

#ifndef CSERVER_CSERVERCONF_H
#define CSERVER_CSERVERCONF_H


#include <string>

#include <spdlog/common.h>

#include <LuaServer.h>

extern size_t data_volume(std::string volstr);

//void cserverConfGlobals(lua_State *L, void *user_data);

extern void cserverConfGlobals(lua_State *L, cserve::Connection &conn, void *user_data);

class CserverConf {
private:
    int _serverconf_ok;
    std::string _userid;
    int _port;
    std::string _ssl_certificate;
    std::string _ssl_key;
    std::string _jwt_secret;
    int _ssl_port;
    int _nthreads;
    std::string _docroot;
    std::string  _filehandler_route;
    std::string _tmpdir;
    std::string _scriptdir;
    std::vector <cserve::LuaRoute> _routes; // ToDo: is this necessary??
    std::pair <std::string, std::string> _filehandler_info; // ToDo: is this necessary??
    int _keep_alive;
    size_t _max_post_size;
    std::string _initscript;
    std::string _logfile = "./cserve.log";
    spdlog::level::level_enum _loglevel = spdlog::level::debug;

public:
    /*!
     * Get the configuration parameters. The configuration parameters are provided by
     * - a lua configuration script
     * - environment variables
     * - command line parameters when launching the cserver
     * The parameter in the configuration scripts do have the lowest priority, the command line
     * parameters the highest.
     *
     * @param argc From main() arguments
     * @param argv From main() arguments
     */
    CserverConf(int argc, char *argv[]);

    /*!
     * Returns the status of the configuration process. If everything is OK, a 0 is returned. Non-zero
     * indicates an error.
     *
     * @return 0 on success, non-zero on failure
     */
    [[nodiscard]] inline bool serverconf_ok() const { return _serverconf_ok; }

    inline void userid(const std::string &userid) { _userid = userid; }
    inline std::string userid() { return _userid; }

    inline void port(int port) { _port = port; }
    [[nodiscard]] inline int port() const { return _port; }

    inline void ssl_certificate(const std::string &ssl_certificate) { _ssl_certificate = ssl_certificate; }
    inline std::string ssl_certificate() { return _ssl_certificate; }

    inline void ssl_key(const std::string &ssl_key) { _ssl_key = ssl_key; }
    inline std::string ssl_key() { return _ssl_key; }

    inline void jwt_secret(const std::string &jwt_secret) { _jwt_secret = jwt_secret; }
    inline std::string jwt_secret() { return _jwt_secret; }
    inline void ssl_port(int ssl_port) { _ssl_port = ssl_port; }
    [[nodiscard]] inline int ssl_port() const { return _ssl_port; }

    inline void nthreads(int nthreads) { _nthreads = nthreads; }
    [[nodiscard]] inline int nthreads() const { return _nthreads; }

    inline void docroot(const std::string &docroot) { _docroot = docroot; }
    inline std::string docroot() { return _docroot; }

    inline void tmpdir(const std::string &tmpdir) { _tmpdir = tmpdir; }
    inline std::string tmpdir() { return _tmpdir; }

    inline void scriptdir(const std::string &scriptdir) { _scriptdir = scriptdir; }
    inline std::string scriptdir() { return _scriptdir; }

    inline void routes(const std::vector <cserve::LuaRoute> &routes) { _routes = routes; }
    inline std::vector <cserve::LuaRoute> routes() { return _routes; }

    inline void filehandler_info(const std::pair <std::string, std::string> &filehandler_info) {
        _filehandler_info = filehandler_info;
    }
    inline std::pair <std::string, std::string> filehandler_info() { return _filehandler_info; }

    inline void keep_alive(int keep_alive) { _keep_alive = keep_alive; }
    [[nodiscard]] inline int keep_alive() const { return _keep_alive; }

    inline void max_post_size(size_t max_post_size) { _max_post_size = max_post_size; }
    inline void max_post_size(const std::string &max_post_size) { _max_post_size = data_volume(max_post_size); }
    [[nodiscard]] inline size_t max_post_size() const { return _max_post_size; }

    inline void initscript(const std::string &initscript) { _initscript = initscript; }
    inline std::string initscript() { return _initscript; }

    inline void logfile(const std::string &logfile) { _logfile = logfile; }
    inline std::string logfile() { return _logfile; }

    inline void loglevel(spdlog::level::level_enum loglevel) { _loglevel = loglevel; }
    inline spdlog::level::level_enum loglevel() { return _loglevel; }
};


#endif //CSERVER_CSERVERCONF_H
