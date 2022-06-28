//
// Created by Lukas Rosenthaler on 15.06.22.
//

#include "PingHandler.h"

namespace cserve {

    const std::string PingHandler::_name = "pinghandler";

    const std::string& PingHandler::name() const {
        return _name;
    }

    void PingHandler::handler(cserve::Connection &conn, cserve::LuaServer &lua, void *user_data) {
        conn.header("Content-Type", "text/html; charset=utf-8");
        conn.setBuffer();
        conn << _echo << cserve::Connection::flush_data;
    }

    void PingHandler::set_config_variables(CserverConf &conf) {
        std::vector<LuaRoute> routes = {
                LuaRoute("GET:/ping:C++"),
        };
        std::string routeopt = _name + "_route";
        conf.add_config(_name, routeopt,routes, "Route for handler");
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
