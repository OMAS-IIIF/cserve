//
// Created by Lukas Rosenthaler on 08.06.22.
//

#include "PingHandler.h"

void PingHandler::handler(cserve::Connection &conn, cserve::LuaServer &lua, void *user_data) {
    conn.setBuffer();
    conn << "PONG" << cserve::Connection::flush_data;
}
