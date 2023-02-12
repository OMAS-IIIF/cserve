/*
 * Copyright © 2022 Lukas Rosenthaler
 * This file is part of OMAS/cserve
 * OMAS/cserve is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * OMAS/cserve is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef CSERVER_CSERVERCONF_H
#define CSERVER_CSERVERCONF_H


#include <string>
#include <optional>
#include <utility>

#include "spdlog/common.h"

#include "LuaServer.h"
#include "ConfValue.h"


//void cserverConfGlobals(lua_State *L, void *user_data);

namespace cserve {
    extern void cserverConfGlobals(lua_State *L, cserve::Connection &conn, void *user_data);

    class CserverConf {
    private:
        int _serverconf_ok;
        std::string lua_global_name;
        std::unordered_map<std::string, std::shared_ptr<cserve::ConfValue>> _values;
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
        explicit CserverConf(std::string lua_global_name = "config");

        void add_config(const std::string &prefix, const std::string &name, bool defaultval, const std::string &description);

        void add_config(const std::string &prefix, const std::string &name, int defaultval, const std::string &description);

        void add_config(const std::string &prefix, const std::string& name, float defaultval, const std::string &description);

        void add_config(const std::string &prefix, const std::string& name, const char *defaultval, const std::string &description);

        void add_config(const std::string &prefix, const std::string& name, const std::string &defaultval, const std::string &description);

        void add_config(const std::string &prefix, const std::string& name, const cserve::DataSize &defaultval, const std::string &description);

        void add_config(const std::string &prefix, const std::string& name, spdlog::level::level_enum defaultval, const std::string &description);

        void add_config(const std::string &prefix, const std::string& name, const std::vector<cserve::RouteInfo>& defaultval, const std::string &description);

        void parse_cmdline_args(int argc, const char *argv[]);

        /*!
         * Returns the status of the configuration process. If everything is OK, a 0 is returned. Non-zero
         * indicates an error.
         *
         * @return 0 on success, non-zero on failure
         */
        [[nodiscard]] inline bool serverconf_ok() const { return _serverconf_ok; }
        [[nodiscard]] inline std::string get_lua_global_name() const { return lua_global_name; }

        inline const std::unordered_map<std::string, std::shared_ptr<cserve::ConfValue>> &get_values() { return _values; }

        [[nodiscard]] std::optional<bool> get_bool(const std::string &name) const;

        [[nodiscard]] std::optional<int> get_int(const std::string &name) const;

        [[nodiscard]] std::optional<float> get_float(const std::string &name) const;

        [[nodiscard]] std::optional<std::string> get_string(const std::string &name) const;

        [[nodiscard]] std::optional<cserve::DataSize> get_datasize(const std::string &name) const;

        [[nodiscard]] std::optional<std::vector<cserve::RouteInfo>> get_luaroutes(const std::string &prefix, const std::string &name) const;

        [[nodiscard]] std::optional<spdlog::level::level_enum> get_loglevel(const std::string &name) const;
    };

}



#endif //CSERVER_CSERVERCONF_H
