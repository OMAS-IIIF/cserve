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
