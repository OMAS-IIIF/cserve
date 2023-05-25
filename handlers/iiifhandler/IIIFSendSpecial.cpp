//
// Created by Lukas Rosenthaler on 10.05.23.
//
#include "nlohmann/json.hpp"

#include "IIIFHandler.h"
#include "iiifparser/IIIFIdentifier.h"
static const char file_[] = __FILE__;

namespace cserve {

    void IIIFHandler::send_iiif_special(Connection &conn, LuaServer &luaserver,
                                     const std::unordered_map<Parts, std::string> &params,
                                     const std::string &lua_function_name) const {
        IIIFIdentifier sid{urldecode(params.at(IIIF_IDENTIFIER))};

        if (luaserver.luaFunctionExists(lua_function_name)) {
            // The paramters to be passed to the pre-flight function.
            std::vector<LuaValstruct> lvals;

            // The first parameter is the IIIF prefix.
            lvals.emplace_back(params.at(IIIF_PREFIX));

            // The second parameter is the IIIF identifier.
            lvals.emplace_back(sid.get_identifier());

            // The third parameter is the HTTP cookie.
            lvals.emplace_back(conn.header("cookie"));

            try {
                std::vector<LuaValstruct> rvals = luaserver.executeLuafunction(lua_function_name, lvals);
            }
            catch (const Error &err) {
                std::cerr << "&&?? ERROR: " << err << std::endl;
                nlohmann::json result = {{"result", "error"},{"error", err.to_string()}};
            }
        }
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