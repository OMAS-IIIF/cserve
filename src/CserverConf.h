//
// Created by Lukas Rosenthaler on 28.06.21.
//

#ifndef CSERVER_CSERVERCONF_H
#define CSERVER_CSERVERCONF_H


#include <string>
#include <optional>
#include <utility>

#include <spdlog/common.h>

#include <LuaServer.h>
#include <ConfValue.h>


//void cserverConfGlobals(lua_State *L, void *user_data);

extern void cserverConfGlobals(lua_State *L, cserve::Connection &conn, void *user_data);


class CserverConf {
private:
    int _serverconf_ok;

    std::vector<cserve::LuaRoute> _routes; // ToDo: is this necessary??
    spdlog::level::level_enum _loglevel = spdlog::level::debug;

    std::unordered_map<std::string, cserve::ConfValue> _values;
    std::shared_ptr<CLI::App> _cserverOpts;

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

    void parse_cmdline_args(int argc, char *argv[]);

    /*!
     * Returns the status of the configuration process. If everything is OK, a 0 is returned. Non-zero
     * indicates an error.
     *
     * @return 0 on success, non-zero on failure
     */
    [[nodiscard]] inline bool serverconf_ok() const { return _serverconf_ok; }

    inline const std::unordered_map<std::string, cserve::ConfValue> &get_values() { return _values; }

    std::optional<int> get_int(const std::string &name);

    std::optional<float> get_float(const std::string &name);

    std::optional<std::string> get_string(const std::string &name);

    inline void routes(const std::vector<cserve::LuaRoute> &routes) { _routes = routes; }

    inline std::vector<cserve::LuaRoute> routes() { return _routes; }

    inline void loglevel(spdlog::level::level_enum loglevel) { _loglevel = loglevel; }

    inline spdlog::level::level_enum loglevel() { return _loglevel; }
};


#endif //CSERVER_CSERVERCONF_H
