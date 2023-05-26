//
// Created by Lukas Rosenthaler on 08.07.22.
//

static const char file_[] = __FILE__;

#include "../../lib/LuaServer.h"
#include "IIIFHandler.h"
#include "IIIFError.h"

namespace cserve {

    std::unordered_map<std::string, std::string> IIIFHandler::call_iiif_preflight(Connection &conn_obj,
                                                                                  LuaServer &luaserver,
                                                                                  const std::string &prefix,
                                                                                  const std::string &identifier) const {
        // The permission and optional file path that the pre_fight function returns.
        std::unordered_map<std::string, std::string> preflight_info;
        // std::string permission;
        // std::string infile;

        // The paramters to be passed to the pre-flight function.
        std::vector<LuaValstruct> lvals;

        // The first parameter is the IIIF prefix.
        lvals.emplace_back(prefix);

        // The second parameter is the IIIF identifier.
        lvals.emplace_back(identifier);

        // The third parameter is the HTTP cookie.
        lvals.emplace_back(conn_obj.header("cookie"));

        // Call the pre-flight function.
        std::vector<LuaValstruct> rvals = luaserver.executeLuafunction(_iiif_preflight_funcname, lvals);

        // If it returned nothing, that's an error.
        if (rvals.empty()) {
            throw IIIFError(file_, __LINE__, fmt::format("Lua function '{}' must return at least one value", _iiif_preflight_funcname));
        }

        // The first return value is the permission code.
        auto permission_return_val = rvals.at(0);

        // The permission code must be a string.
        if (permission_return_val.get_type() == LuaValstruct::STRING_TYPE) {
            preflight_info["type"] = permission_return_val.get_string().value();
        } else if (permission_return_val.get_type() == LuaValstruct::TABLE_TYPE) {
            try {
                std::unordered_map<std::string, LuaValstruct> tmpmap(permission_return_val.get_table().value());
                preflight_info["type"] = tmpmap.at("type").get_string().value();
                for (const auto &[key, val]: tmpmap) {
                    if (key == "type") continue;
                    preflight_info[key] = val.get_string().value();
                }
            }
            catch (const std::bad_optional_access &err) {
                throw IIIFError(file_, __LINE__,fmt::format("The permission value returned by Lua function '{}' is not of string type", _iiif_preflight_funcname));
            }
            catch (const std::out_of_range &err) {
                throw IIIFError(file_, __LINE__,fmt::format("The permission value returned by Lua function '{}' has no type field", _iiif_preflight_funcname));
            }
        } else {
            throw IIIFError(file_, __LINE__, fmt::format("The permission value returned by Lua function '{}' was not valid", _iiif_preflight_funcname));
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
            throw IIIFError(file_, __LINE__, fmt::format("The permission returned by Lua function '{}' is not valid: {}", _iiif_preflight_funcname, preflight_info["type"]));
        }

        if (preflight_info["type"] == "deny") {
            preflight_info["infile"] = "";
        } else {
            if (rvals.size() < 2) {
                throw IIIFError(file_, __LINE__, fmt::format("Lua function {} returned other permission than 'deny', but it did not return a file path", _iiif_preflight_funcname));
            }

            // The file path must exist and be a string.
            try {
                auto infile_return_val = rvals.at(1);
                preflight_info["infile"] = infile_return_val.get_string().value();
            }
            catch (const std::bad_optional_access &err) {
                throw IIIFError(file_, __LINE__,fmt::format("The file path returned by Lua function  '{}'  was not a string", _iiif_preflight_funcname));
            }
            catch (const std::out_of_range &err) {
                throw IIIFError(file_, __LINE__,fmt::format("The file path returned by Lua function  '{}'  was not a string", _iiif_preflight_funcname));
            }
        }

        // Return the permission code and file path, if any, as a std::pair.
        return preflight_info;
    }

    std::unordered_map<std::string, std::string> IIIFHandler::call_blob_preflight(Connection &conn_obj,
                                                                                  LuaServer &luaserver,
                                                                                  const std::string &filepath) const {
        // The permission and optional file path that the pre_fight function returns.
        std::unordered_map<std::string, std::string> preflight_info;
        // std::string permission;
        // std::string infile;

        // The paramters to be passed to the pre-flight function.
        std::vector<LuaValstruct> lvals;

        // The first parameter is the filepath.
        lvals.emplace_back(filepath);

        // The second parameter is the HTTP cookie.
        lvals.emplace_back(conn_obj.header("cookie"));

        // Call the pre-flight function.
        std::vector<LuaValstruct> rvals = luaserver.executeLuafunction(_file_preflight_funcname, lvals);

        // If it returned nothing, that's an error.
        if (rvals.empty()) {
            throw IIIFError(file_, __LINE__, fmt::format("Lua function '{}' must return at least one value", _file_preflight_funcname));
        }

        // The first return value is the permission code.
        auto permission_return_val = rvals.at(0);

        // The permission code must be a string or a table.
        if (permission_return_val.get_type() == LuaValstruct::STRING_TYPE) {
            preflight_info["type"] = permission_return_val.get_string().value();
        } else if (permission_return_val.get_type() == LuaValstruct::TABLE_TYPE) {
            try {
                std::unordered_map<std::string, LuaValstruct> tmpmap(permission_return_val.get_table().value());
                preflight_info["type"] = tmpmap.at("type").get_string().value();
                for (const auto &[key, val]: tmpmap) {
                    if (key == "type") continue;
                    preflight_info[key] = val.get_string().value();
                }
            }
            catch (const std::bad_optional_access &err) {
                throw IIIFError(file_, __LINE__,fmt::format("The permission value returned by Lua function '{}' is not of string type", _iiif_preflight_funcname));
            }
            catch (const std::out_of_range &err) {
                throw IIIFError(file_, __LINE__,fmt::format("The permission value returned by Lua function '{}' has no type field", _iiif_preflight_funcname));
            }
        } else {
            throw IIIFError(file_, __LINE__, fmt::format("The permission value returned by Lua function '{}' was not valid", _iiif_preflight_funcname));
        }

        //
        // check if permission type is valid
        //
        if ((preflight_info["type"] != "allow") &&
            (preflight_info["type"] != "login") &&
            (preflight_info["type"] != "restrict") &&
            (preflight_info["type"] != "deny")) {
             throw IIIFError(file_, __LINE__, fmt::format("The permission returned by Lua function {} is not valid: {}", _file_preflight_funcname, preflight_info["type"]));
        }

        if (preflight_info["type"] == "deny") {
            preflight_info["infile"] = "";
        }
        else {
            // The file path must be a string.
            try {
                auto infile_return_val = rvals.at(1);
                preflight_info["infile"] = infile_return_val.get_string().value();
            }
            catch (const std::bad_optional_access &err) {
                throw IIIFError(file_, __LINE__,fmt::format("The file path returned by Lua function  '{}'  was not a string", _iiif_preflight_funcname));
            }
            catch (const std::out_of_range &err) {
                throw IIIFError(file_, __LINE__,fmt::format("The file path returned by Lua function  '{}'  was not a string", _iiif_preflight_funcname));
            }
        }

        // Return the permission code and file path, if any, as a std::pair.
        return preflight_info;
    }


}
