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

        std::vector<LuaValstruct> rvals;
        nlohmann::json result;
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
                rvals = luaserver.executeLuafunction(lua_function_name, lvals);
            }
            catch (const Error &err) {
                result = {{"status", "ERROR"}, {"errormsg", err.to_string()}};
            }
        }
        if (rvals.size() != 1) {
            result = {{"status", "ERROR"}, {"errormsg", "More than one return value from script."}};
        }
        if (result.empty()) {
            result["status"] = "OK";
            result["result"] = rvals[0].get_json();
        }
        std::string json_str = result.dump(3);

        Connection::StatusCodes http_status = Connection::StatusCodes::OK;
        conn.status(http_status);
        conn.setBuffer(); // we want buffered output, since we send JSON text...
        conn.header("Access-Control-Allow-Origin", "*");
        conn.sendAndFlush(json_str.c_str(), json_str.size());
    }

}