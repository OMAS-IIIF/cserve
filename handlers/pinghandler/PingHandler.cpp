//
// Created by Lukas Rosenthaler on 15.06.22.
//

#include "PingHandler.h"

namespace cserve {
    void PingHandler::handler(cserve::Connection &conn, cserve::LuaServer &lua, void *user_data) {
        conn.header("Content-Type", "text/html; charset=utf-8");
        conn.setBuffer();
        conn << "PONG" << cserve::Connection::flush_data;
    }
} // cserve

extern "C" cserve::PingHandler * createPingHandler() {
    return new cserve::PingHandler();
};

extern "C" void destroyPingHandler(cserve::PingHandler *handler) {
    delete handler;
}
