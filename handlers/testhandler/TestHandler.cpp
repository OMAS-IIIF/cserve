//
// Created by Lukas Rosenthaler on 15.06.22.
//

#include "TestHandler.h"
#include "../../lib/CserveVersion.h"

namespace cserve {

    const std::string TestHandler::_name = "testhandler";

    const std::string& TestHandler::name() const {
        return _name;
    }

    void TestHandler::handler(Connection &conn, LuaServer &lua, const std::string &route) {
        conn.setBuffer();
        conn.setChunkedTransfer();

        conn.header("Content-Type", "text/html; charset=utf-8");
        conn << "<html><head>";
        conn << "<title>OMAS CSERVE V"
             << std::to_string(cserver_VERSION_MAJOR) << "."
             << std::to_string(cserver_VERSION_MINOR) << "."
             << std::to_string(cserver_VERSION_PATCH) << "</title>";
        conn << "</head>" << cserve::Connection::flush_data;

        conn << "<body><h1>OMAS CSERVE V"
             << std::to_string(cserver_VERSION_MAJOR) << "."
             << std::to_string(cserver_VERSION_MINOR) << "."
             << std::to_string(cserver_VERSION_PATCH) << "</h1>";

        conn << "<p>" << _message << "</p>";
        conn << "</body></html>" << cserve::Connection::flush_data;
    }

    void TestHandler::set_config_variables(CserverConf &conf) {
        //conf.append_route(RouteInfo("GET:/test:C++"))
        std::vector<RouteInfo> routes = {
                RouteInfo("GET:/test:C++"),
        };
        //std::string routeopt = _name + "_route";
        conf.add_config(_name, "routes",routes, "Route for handler");
        conf.add_config(_name, "message", "Hello World", "Message to display");
    }

    void TestHandler::get_config_variables(const CserverConf &conf) {
        _message = conf.get_string("message").value_or("-- no message --");
    }

} // cserve
