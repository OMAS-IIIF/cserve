//
// Created by Lukas Rosenthaler on 23.06.22.
//

#include "NlohmannTraits.h"
#include <utility>
#include <nlohmann/json.hpp>
#include <jwt-cpp/jwt.h>
#include "sole.hpp"
#include "Error.h"
#include "Cserve.h"
#include "Connection.h"
#include "LuaServer.h"
#include "Parsing.h"
#include "curl/curl.h"
#include <dirent.h>
#include "spdlog/spdlog.h"
#include <unistd.h>
#include <sys/stat.h>
#include <cerrno>
#include <chrono>
#include <cstring>      // Needed for memset
#include <string>
#include <iostream>
#include <cctype>
#include <functional>
#include "LuaConfig.h"

/*!
 * Reads a string item from a Lua config file. A lua config file is always a table with
 * a name (e.g. sipi which contains configuration parameters
 *
 * @param table Table name for config parameters
 * @param variable Variable name
 * @param defval Default value to take if variable is not defined
 * @return Configuartion parameter value
 */
std::string cserve::LuaServer::configString(const std::string& table, const std::string& variable, const std::string& defval) {
    if (lua_getglobal(L, table.c_str()) != LUA_TTABLE) {
        lua_pop(L, 1);
        return defval;
    }

    lua_getfield(L, -1, variable.c_str());

    if (lua_isnil(L, -1)) {
        lua_pop(L, 2);
        return defval;
    }

    if (!lua_isstring(L, -1)) {
        throw Error(file_, __LINE__, "String expected for " + table + "." + variable);
    }

    std::string retval = lua_tostring(L, -1);
    lua_pop(L, 2);
    return retval;
}

/*!
 * Reads a boolean item from a Lua config file. A lua config file is always a table with
 * a name (e.g. sipi which contains configuration parameters
 *
 * @param table Table name for config parameters
 * @param variable Variable name
 * @param defval Default value to take if variable is not defined
 * @return Configuartion parameter value
 */
[[maybe_unused]] bool cserve::LuaServer::configBoolean(const std::string& table, const std::string& variable, const bool defval) {
    if (lua_getglobal(L, table.c_str()) != LUA_TTABLE) {
        lua_pop(L, 1);
        return defval;
    }

    lua_getfield(L, -1, variable.c_str());

    if (lua_isnil(L, -1)) {
        lua_pop(L, 2);
        return defval;
    }

    if (!lua_isboolean(L, -1)) {
        throw Error(file_, __LINE__, "Boolean expected for " + table + "." + variable);
    }

    bool retval = lua_toboolean(L, -1) == 1;
    lua_pop(L, 2);
    return retval;
}

/*!
 * Reads a boolean item from a Lua config file. A lua config file is always a table with
 * a name (e.g. sipi which contains configuration parameters
 *
 * @param table Table name for config parameters
 * @param variable Variable name
 * @param defval Default value to take if variable is not defined
 * @return Configuartion parameter value
 */
int cserve::LuaServer::configInteger(const std::string& table, const std::string& variable, const int defval) {
    if (lua_getglobal(L, table.c_str()) != LUA_TTABLE) {
        lua_pop(L, 1);
        return defval;
    }

    lua_getfield(L, -1, variable.c_str());

    if (lua_isnil(L, -1)) {
        lua_pop(L, 2);
        return defval;
    }

    if (!lua_isinteger(L, -1)) {
        throw Error(file_, __LINE__, "Integer expected for " + table + "." + variable);
    }

    int retval = static_cast<int>(lua_tointeger(L, -1));
    lua_pop(L, 2);
    return retval;
}

spdlog::level::level_enum cserve::LuaServer::configLoglevel(const std::string& table, const std::string& variable, spdlog::level::level_enum defval) {
    if (lua_getglobal(L, table.c_str()) != LUA_TTABLE) {
        lua_pop(L, 1);
        return defval;
    }

    lua_getfield(L, -1, variable.c_str());

    if (lua_isnil(L, -1)) {
        lua_pop(L, 2);
        return defval;
    }

    if (!lua_isstring(L, -1)) {
        throw Error(file_, __LINE__, "\"TRACE\", \"DEBUG\", \"INFO\", \"WARN\", \"ERR\", \"CRITICAL\" or \"OFF\" expected for " + table + "." + variable);
    }

    std::string loglevel_str = lua_tostring(L, -1);
    std::unordered_map<std::string, spdlog::level::level_enum> loglevel_map{
            {"TRACE",    spdlog::level::trace},
            {"DEBUG",    spdlog::level::debug},
            {"INFO",     spdlog::level::info},
            {"WARN",     spdlog::level::warn},
            {"ERR",      spdlog::level::err},
            {"CRITICAL", spdlog::level::critical},
            {"OFF",      spdlog::level::off}
    };

    spdlog::level::level_enum loglevel;
    try {
        loglevel = loglevel_map.at(loglevel_str);
    }
    catch (const std::out_of_range &err) {
        throw Error(file_, __LINE__, "\"TRACE\", \"DEBUG\", \"INFO\", \"WARN\", \"ERR\", \"CRITICAL\" or \"OFF\" expected for " + table + "." + variable);
    }

    lua_pop(L, 2);
    return loglevel;
}

/*!
 * Reads a float item from a Lua config file. A lua config file is always a table with
 * a name (e.g. sipi which contains configuration parameters
 *
 * @param table Table name for config parameters
 * @param variable Variable name
 * @param defval Default value to take if variable is not defined
 * @return Configuartion parameter value
 */
float cserve::LuaServer::configFloat(const std::string& table, const std::string& variable, const float defval) {
    if (lua_getglobal(L, table.c_str()) != LUA_TTABLE) {
        lua_pop(L, 1);
        return defval;
    }

    lua_getfield(L, -1, variable.c_str());

    if (lua_isnil(L, -1)) {
        lua_pop(L, 2);
        return defval;
    }

    if (!lua_isnumber(L, -1)) {
        throw Error(file_, __LINE__, "Number expected for " + table + "." + variable);
    }

    lua_Number num = lua_tonumber(L, -1);
    lua_pop(L, 2);
    return (float) num;
}

/*!
 * Reads a table of values and returns the values as string vector
 *
 * @param table Table name for config parameters
 * @param variable Variable name (which must be a table/array)
 * @return Vector of table entries
 */
std::__1::vector<std::string> cserve::LuaServer::configStringList(const std::string& table, const std::string& variable) {
    std::__1::vector<std::string> strings;
    if (lua_getglobal(L, table.c_str()) != LUA_TTABLE) {
        lua_pop(L, 1);
        return strings;
    }

    lua_getfield(L, -1, variable.c_str());

    if (lua_isnil(L, -1)) {
        lua_pop(L, 2);
        return strings;
    }

    if (!lua_istable(L, -1)) {
        throw Error(file_, __LINE__, "Value '" + variable + "' in config file must be a table");
    }

    for (int i = 1;; i++) {
        lua_rawgeti(L, -1, i);

        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            break;
        }
        if (lua_isstring(L, -1)) {
            std::string tmpstr = lua_tostring(L, -1);
            strings.push_back(tmpstr);
        }
        lua_pop(L, 1);
    }
    return strings;
}

/*!
 * Reads a table with key value pairs
 * @param table Table name for config parameters
 * @param variable Variable name (which must be a table with key value pairs)
 * @return Map of key-value pairs
 */
std::__1::map<std::string,std::string> cserve::LuaServer::configStringTable(
        const std::string &table,
        const std::string &variable,
        const std::__1::map<std::string,std::string> &defval) {
    std::__1::map<std::string,std::string> subtable;

    int top = lua_gettop(L);

    if (lua_getglobal(L, table.c_str()) != LUA_TTABLE) {
        lua_pop(L, top);
        return defval;
    }

    try {
        lua_getfield(L, 1, variable.c_str());
    }
    catch(...) {
        return defval;
    }

    if (lua_isnil(L, -1)) {
        lua_pop(L, lua_gettop(L));
        return defval;
    }

    if (!lua_istable(L, -1)) {
        throw Error(file_, __LINE__, "Value '" + variable + "' in config file must be a table");
    }

    lua_pushvalue(L, -1);
    lua_pushnil(L);

    while (lua_next(L, -2)) {
        std::string valstr, keystr;
        lua_pushvalue(L, -2);
        if (lua_isstring(L, -1)) {
            keystr = lua_tostring(L, -1);
        }
        else {
            throw Error(file_, __LINE__, "Key element of '" + variable + "' in config file must be a string");
        }
        if (lua_isstring(L, -2)) {
            valstr = lua_tostring(L, -2);
        }
        else {
            throw Error(file_, __LINE__, "Value element of '" + variable + "' in config file must be a string");
        }
        lua_pop(L, 2);
        subtable[keystr] = valstr;
    }

    return subtable;
}

/*!
 * Read a route definition (containing "method", "route", "script" keys)
 *
 * @param routetable A table name containing route info
 * @return Route info
 */
std::__1::vector<cserve::LuaRoute> cserve::LuaServer::configRoute(const std::string& routetable) {
    static struct {
        const char *name;
        int type;
    } fields[] = {{"method", LUA_TSTRING},
                  {"route",  LUA_TSTRING},
                  {"script", LUA_TSTRING},
                  {nullptr, 0}};

    std::__1::vector<LuaRoute> routes;

    lua_getglobal(L, routetable.c_str());

    if (!lua_istable(L, -1)) {
        throw Error(file_, __LINE__, "Value '" + routetable + "' in config file must be a table");
    }

    for (int i = 1;; i++) {
        lua_rawgeti(L, -1, i);

        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            break;
        }

        // an element of the 'listen' table should now be at the top of the stack
        luaL_checktype(L, -1, LUA_TTABLE);
        int field_index;
        LuaRoute route;

        for (field_index = 0; fields[field_index].name != nullptr; field_index++) {
            lua_getfield(L, -1, fields[field_index].name);
            luaL_checktype(L, -1, fields[field_index].type);

            std::string method;
            // you should probably use a function pointer in the fields table.
            // I am using a simple switch/case here
            switch (field_index) {
                case 0:
                    method = lua_tostring(L, -1);
                    if (method == "GET") {
                        route.method = Connection::GET;
                    } else if (method == "PUT") {
                        route.method = Connection::PUT;
                    } else if (method == "POST") {
                        route.method = Connection::POST;
                    } else if (method == "DELETE") {
                        route.method = Connection::DELETE;
                    } else if (method == "OPTIONS") {
                        route.method = Connection::OPTIONS;
                    } else if (method == "CONNECT") {
                        route.method = Connection::CONNECT;
                    } else if (method == "HEAD") {
                        route.method = Connection::HEAD;
                    } else if (method == "OTHER") {
                        route.method = Connection::OTHER;
                    } else {
                        throw Error(file_, __LINE__, std::string("Unknown HTTP method") + method);
                    }
                    break;
                case 1:
                    route.route = lua_tostring(L, -1);
                    break;
                case 2:
                    route.script = lua_tostring(L, -1);
                    break;
            }
            // remove the field value from the top of the stack
            lua_pop(L, 1);
        }

        lua_pop(L, 1);
        routes.push_back(route);
    }

    return routes;
}