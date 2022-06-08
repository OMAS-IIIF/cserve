//
// Created by Lukas Rosenthaler on 08.06.22.
//

#include "TestHandler.h"
#include "CserveVersion.h"

void TestHandler::handler(cserve::Connection &conn, cserve::LuaServer &lua, void *user_data) {
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
