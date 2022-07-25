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
#ifndef CSERVER_REQUESTHANDLER_H
#define CSERVER_REQUESTHANDLER_H

#include "Connection.h"
#include "LuaServer.h"
#include "CserverConf.h"

namespace cserve {

    /*!
     * Abstract class for defining a request handler
     */
    class RequestHandler {
    protected:
        std::unordered_map<std::string,std::string> routedata;
        std::string _route;
    public:

        /*!
         * Default constructur
         */
        RequestHandler() = default;
        
        /*!
         * Default destructor
         */
        virtual ~RequestHandler() = default;

        virtual const std::string& name() const = 0;

        inline void set_route(const std::string &route) { _route = route; }

        inline void add_route_data(const std::string &route, const std::string &data) {
            routedata[route] = data;
        }

        inline std::string get_route() { return _route; }

        /*!
         * Pure virtual function that gives the template for implementing a request handler
         * @param conn Connection reference
         * @param lua Lua interpreter
         * @param user_data Additional unspecified data that can be given to the handler
         */
        virtual void handler(Connection& conn, LuaServer& lua, const std::string &route) = 0;

        virtual inline void set_config_variables(CserverConf &conf) {}

        virtual inline void get_config_variables(const CserverConf &conf) {}

        virtual inline void set_lua_globals(lua_State *L, cserve::Connection &conn) {}

    };

} // cserve

#endif //CSERVER_REQUESTHANDLER_H
