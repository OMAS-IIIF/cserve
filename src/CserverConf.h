//
// Created by Lukas Rosenthaler on 28.06.21.
//

#ifndef CSERVER_CSERVERCONF_H
#define CSERVER_CSERVERCONF_H


#include <string>

#include <spdlog/common.h>

#include <LuaServer.h>

extern size_t data_volume(const std::string volstr);

class CserverConf {
    std::string _userid;
    int _port;
#ifdef CSERVE_ENABLE_SSL
    std::string _ssl_certificate;
    std::string _ssl_key;
    std::string _jwt_secret;
    int _ssl_port = 4712;
#else
    int _ssl_port = -1;
#endif
    int _nthreads;
    std::string _docroot;
    std::string _tmpdir;
    std::string _scriptdir;
    std::vector <cserve::LuaRoute> routes; // ToDo: is this necessary??
    std::pair <std::string, std::string> filehandler_info; // ToDo: is this necessary??
    int _keep_alive = 5;
    size_t _max_post_size = 20*1024*1024; // 20MB
    std::string _initscript = "./cserve.init.lua";
    std::string _logfile = "./cserve.log";
    spdlog::level::level_enum _loglevel = spdlog::level::debug;


    CserverConf(int argc, char *argv[]);



    inline void userid(const std::string &userid) { _userid = userid; }
    inline std::string userid() { return _userid; }

    inline void port(int port) { _port = port; }
    inline int port() { return _port; }

#ifdef CSERVE_ENABLE_SSL
    inline void ssl_certificate(const std::string &ssl_certificate) { _ssl_certificate = ssl_certificate; }
    inline std::string ssl_certificate() { return _ssl_certificate; }

    inline void ssl_key(const std::string &ssl_key) { _ssl_key = ssl_key; }
    inline std::string ssl_key() { return _ssl_key; }

    inline void jwt_secret(const std::string &jwt_secret) { _jwt_secret = jwt_secret; }
    inline std::string jwt_secret() { return _jwt_secret; }
#endif
    inline void ssl_port(int ssl_port) { _ssl_port = ssl_port; }
    inline int ssl_port() { return _ssl_port; }

    inline void nthreads(int nthreads) { _nthreads = nthreads; }
    inline int nthreads() { return _nthreads; }

    inline void docroot(const std::string &docroot) { _docroot = docroot; }
    inline std::string docroot() { return _docroot; }

    inline void tmpdir(const std::string &tmpdir) { _tmpdir = tmpdir; }
    inline std::string tmpdir() { return _tmpdir; }

    inline void keep_alive(int keep_alive) { _keep_alive = keep_alive; }
    inline int keep_alive() { return _keep_alive; }

    inline void max_post_size(size_t max_post_size) { _max_post_size = max_post_size; }
    inline void max_post_size(const std::string &max_post_size) { _max_post_size = data_volume(max_post_size); }
    inline size_t max_post_size() { return _max_post_size; }

    inline void initscript(const std::string &initscript) { _initscript = initscript; }
    inline std::string initscript() { return _initscript; }

    inline void logfile(const std::string &logfile) { _logfile = logfile; }
    inline std::string logfile() { return _logfile; }

    inline void loglevel(spdlog::level::level_enum loglevel) { _loglevel = loglevel; }
    inline spdlog::level::level_enum loglevel() { return _loglevel; }
};


#endif //CSERVER_CSERVERCONF_H
