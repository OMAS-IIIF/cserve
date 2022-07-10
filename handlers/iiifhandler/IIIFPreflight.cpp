//
// Created by Lukas Rosenthaler on 08.07.22.
//

const static char file_[] = __FILE__;

#include "IIIFHandler.h"
#include "IIIFError.h"

namespace cserve {

    std::unordered_map<std::string, std::string> IIIFHandler::call_pre_flight(Connection &conn_obj,
                                                                              LuaServer &luaserver,
                                                                              const std::string &prefix,
                                                                              const std::string &identifier) {
        // The permission and optional file path that the pre_fight function returns.
        std::unordered_map<std::string, std::string> preflight_info;
        // std::string permission;
        // std::string infile;

        // The paramters to be passed to the pre-flight function.
        std::vector<std::shared_ptr<LuaValstruct>> lvals;

        // The first parameter is the IIIF prefix.
        std::shared_ptr<LuaValstruct> iiif_prefix_param = std::make_shared<LuaValstruct>();
        iiif_prefix_param->type = LuaValstruct::STRING_TYPE;
        iiif_prefix_param->value.s = prefix;
        lvals.push_back(iiif_prefix_param);

        // The second parameter is the IIIF identifier.
        std::shared_ptr<LuaValstruct> iiif_identifier_param = std::make_shared<LuaValstruct>();
        iiif_identifier_param->type = LuaValstruct::STRING_TYPE;
        iiif_identifier_param->value.s = identifier;
        lvals.push_back(iiif_identifier_param);

        // The third parameter is the HTTP cookie.
        std::shared_ptr<LuaValstruct> cookie_param = std::make_shared<LuaValstruct>();
        std::string cookie = conn_obj.header("cookie");
        cookie_param->type = LuaValstruct::STRING_TYPE;
        cookie_param->value.s = cookie;
        lvals.push_back(cookie_param);

        // Call the pre-flight function.
        std::vector<std::shared_ptr<LuaValstruct>> rvals = luaserver.executeLuafunction(_pre_flight_func_name, lvals);

        // If it returned nothing, that's an error.
        if (rvals.empty()) {
            std::ostringstream err_msg;
            err_msg << "Lua function " << _pre_flight_func_name << " must return at least one value";
            throw IIIFError(file_, __LINE__, err_msg.str());
        }

        // The first return value is the permission code.
        auto permission_return_val = rvals.at(0);

        // The permission code must be a string.
        if (permission_return_val->type == LuaValstruct::STRING_TYPE) {
            preflight_info["type"] = permission_return_val->value.s;
        } else if (permission_return_val->type == LuaValstruct::TABLE_TYPE) {
            std::shared_ptr<LuaValstruct> tmpv;
            try {
                tmpv = permission_return_val->value.table.at("type");
            }
            catch (const std::out_of_range &err) {
                std::ostringstream err_msg;
                err_msg << "The permission value returned by Lua function " << _pre_flight_func_name
                        << " has no type field!";
                throw IIIFError(file_, __LINE__, err_msg.str());
            }
            if (tmpv->type != LuaValstruct::STRING_TYPE) {
                throw IIIFError(file_, __LINE__, "String value expected!");
            }
            preflight_info["type"] = tmpv->value.s;
            for (const auto &keyval: permission_return_val->value.table) {
                if (keyval.first == "type")
                    continue;
                if (keyval.second->type != LuaValstruct::STRING_TYPE) {
                    throw IIIFError(file_, __LINE__, "String value expected!");
                }
                preflight_info[keyval.first] = keyval.second->value.s;
            }
        } else {
            std::ostringstream err_msg;
            err_msg << "The permission value returned by Lua function " << _pre_flight_func_name << " was not valid";
            throw IIIFError(file_, __LINE__, err_msg.str());
        }

        //
        // check if permission type is valid
        //
        if ((preflight_info["type"] != "allow") &&
            (preflight_info["type"] != "login") &&
            (preflight_info["type"] != "clickthrough") &&
            (preflight_info["type"] != "kiosk") &&
            (preflight_info["type"] != "external") &&
            (preflight_info["type"] != "restrict") &&
            (preflight_info["type"] != "deny")) {
            std::ostringstream err_msg;
            err_msg << "The permission returned by Lua function " << _pre_flight_func_name << " is not valid: "
                    << preflight_info["type"];
            throw IIIFError(file_, __LINE__, err_msg.str());
        }

        if (preflight_info["type"] == "deny") {
            preflight_info["infile"] = "";
        } else {
            if (rvals.size() < 2) {
                std::ostringstream err_msg;
                err_msg << "Lua function " << _pre_flight_func_name
                        << " returned other permission than 'deny', but it did not return a file path";
                throw IIIFError(file_, __LINE__, err_msg.str());
            }

            auto infile_return_val = rvals.at(1);

            // The file path must be a string.
            if (infile_return_val->type == LuaValstruct::STRING_TYPE) {
                preflight_info["infile"] = infile_return_val->value.s;
            } else {
                std::ostringstream err_msg;
                err_msg << "The file path returned by Lua function " << _pre_flight_func_name << " was not a string";
                throw IIIFError(file_, __LINE__, err_msg.str());
            }
        }

        // Return the permission code and file path, if any, as a std::pair.
        return preflight_info;
    }

}
