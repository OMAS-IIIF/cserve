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
#include "../../../lib/Cserve.h"
#include "PingHandler.h"

namespace cserve {

    const std::string PingHandler::_name = "pinghandler";

    const std::string& PingHandler::name() const {
        return _name;
    }

    void PingHandler::handler(Connection &conn, LuaServer &lua, const std::string &route) {
        try {
            conn.header("Content-Type", "text/text; charset=utf-8");
            conn.setBuffer();
            conn << _echo << cserve::Connection::flush_data;
        }
        catch (InputFailure &err) {
            Server::logger()->debug("conn write error: {}", err.what());
            return;
        }
    }

    void PingHandler::set_config_variables(CserverConf &conf) {
        std::vector<RouteInfo> routes = {
                RouteInfo("GET:/ping:C++"),
        };
        conf.add_config(_name, "routes",routes, "Route for handler");
        conf.add_config(_name, "echo", "PONG", "Message to echo");
    }

    void PingHandler::get_config_variables(const CserverConf &conf) {
        _echo = conf.get_string("echo").value_or("-- no echo --");
    }

} // cserve

extern "C" cserve::PingHandler * create_pinghandler() {
    return new cserve::PingHandler();
};

extern "C" void destroy_pinghandler(cserve::PingHandler *handler) {
    delete handler;
}
