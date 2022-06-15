//
// Created by Lukas Rosenthaler on 15.06.22.
//

#include "TestHandler.h"
#include "../../lib/CserveVersion.h"

namespace cserve {

    void TestHandler::handler(Connection &conn, LuaServer &lua, void *user_data) {
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

        conn << "<p>It works!</p>";
        conn << "</body></html>" << cserve::Connection::flush_data;

    }

} // cserve

extern "C" cserve::TestHandler * createTestHandler() {
    return new cserve::TestHandler();
};

extern "C" void destroyTestHandler(cserve::TestHandler *handler) {
    delete handler;
}
