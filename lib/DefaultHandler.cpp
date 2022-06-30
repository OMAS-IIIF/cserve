//
// Created by Lukas Rosenthaler on 08.06.22.
//

#include "Cserve.h"

#include "DefaultHandler.h"

namespace cserve {

    const std::string DefaultHandler::_name = "defaulthandler";

    const std::string& DefaultHandler::name() const {
        return _name;
    }

    void DefaultHandler::handler(Connection &conn, LuaServer &lua, const std::string &route) {
        conn.status(Connection::NOT_FOUND);
        conn.header("Content-Type", "text/text");
        conn.setBuffer();

        try {
            conn << "No handler available" << Connection::flush_data;
        } catch (InputFailure &iofail) {
            Server::logger()->error("No handler and no connection available.");
            return;
        }

        Server::logger()->error("No handler available. Host: '{}' Uri: '{}'", conn.host(), conn.uri());
    }
//=========================================================================

}
