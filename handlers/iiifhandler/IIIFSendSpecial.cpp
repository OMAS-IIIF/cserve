//
// Created by Lukas Rosenthaler on 10.05.23.
//
#include "nlohmann/json.hpp"

#include "IIIFHandler.h"
#include "IIIFError.h"
#include "iiifparser/IIIFIdentifier.h"
static const char file_[] = __FILE__;

namespace cserve {

    void IIIFHandler::send_iiif_special(Connection &conn, LuaServer &luaserver,
                                     const std::unordered_map<Parts, std::string> &params,
                                     const std::string &lua_function_name) const {
        std::cerr << "&&== lua_function_name = " << lua_function_name << std::endl;
        nlohmann::json result = {{"result", "ok"},{"message", lua_function_name}};

        std::string json_str = result.dump(3);

        Connection::StatusCodes http_status = Connection::StatusCodes::OK;
        conn.status(http_status);
        conn.setBuffer(); // we want buffered output, since we send JSON text...
        conn.header("Access-Control-Allow-Origin", "*");
        conn.sendAndFlush(json_str.c_str(), json_str.size());
    }

}